//////////////////////////////////////////////////////////////////////
//
//  NearChuckle on-device diagnostics log.
//
//  Built for users without a PC: everything that matters is captured
//  into one readable file inside the game folder, saved to the public
//  Downloads folder and can be shared straight to a messenger with the
//  in-game LOG button (or automatically after a crash, on next launch).
//
//  diag.txt contains:
//    - device info (model, Android version, CPU, memory)
//    - every SDL_Log line (GLESCompat renderer, bootstrap, ...)
//    - native crash reports (signal, fault address, library map tail)
//  The engine's own log.txt lives next to it and is appended when the
//  combined log is collected/shared.
//
//////////////////////////////////////////////////////////////////////

#if defined(__ANDROID__)

#include <SDL3/SDL.h>
#include <jni.h>
#include <stdarg.h>
#include <sys/system_properties.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <android/log.h>

#define DIAG_MAX_FILE (1500 * 1024)   // rotate diag.txt beyond this
#define DIAG_MAX_COLLECT (160 * 1024) // per-file tail limit when sharing

static int g_diagFd = -1;
static char g_szDiagPath[1024];
static char g_szFilesDir[900];
static bool g_bCrashHandlerInstalled = false;
static bool g_bAutoShareChecked = false;

// ---------------------------------------------------------------------------
// low-level append (async-signal-safe: only ::write / ::open used)
// ---------------------------------------------------------------------------
static void DiagWriteRaw(const char *p, size_t n)
{
  if (g_diagFd >= 0 && p && n)
  {
    ssize_t r = ::write(g_diagFd, p, n);
    (void)r;
  }
}

static void DiagWriteStr(const char *s)
{
  if (s)
    DiagWriteRaw(s, strlen(s));
}

void DiagWrite(const char *fmt, ...)
{
  if (g_diagFd < 0)
    return;
  char buf[2048];
  // timestamp
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  struct tm tmv;
  time_t tt = ts.tv_sec;
  localtime_r(&tt, &tmv);
  int n = snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] ",
                   tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (int)(ts.tv_nsec / 1000000));
  va_list ap;
  va_start(ap, fmt);
  int m = vsnprintf(buf + n, sizeof(buf) - n - 1, fmt, ap);
  va_end(ap);
  if (m < 0)
    return;
  n += (m > (int)sizeof(buf) - n - 2) ? (int)sizeof(buf) - n - 2 : m;
  buf[n++] = '\n';
  DiagWriteRaw(buf, (size_t)n);
}

static void DiagRotateIfNeeded()
{
  if (g_diagFd < 0)
    return;
  struct stat st;
  if (fstat(g_diagFd, &st) == 0 && st.st_size > DIAG_MAX_FILE)
  {
    ::close(g_diagFd);
    g_diagFd = -1;
    char old[1100];
    snprintf(old, sizeof(old), "%s/diag.old.txt", g_szFilesDir);
    rename(g_szDiagPath, old); // keep one generation
    g_diagFd = ::open(g_szDiagPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    DiagWriteStr("=== log rotated ===\n");
  }
}

// ---------------------------------------------------------------------------
// device info
// ---------------------------------------------------------------------------
static void WriteDeviceInfo()
{
  DiagWriteStr("=== NearChuckle run started ===\n");
  time_t tt = time(0);
  char tb[64];
  strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", localtime(&tt));
  DiagWrite("date: %s", tb);
  DiagWrite("game dir: %s", g_szFilesDir);

  char model[PROP_VALUE_MAX + 1] = "?";
  char release[PROP_VALUE_MAX + 1] = "?";
  char sdk[PROP_VALUE_MAX + 1] = "?";
  __system_property_get("ro.product.model", model);
  __system_property_get("ro.build.version.release", release);
  __system_property_get("ro.build.version.sdk", sdk);
  DiagWrite("device: %s, Android %s (SDK %s)", model, release, sdk);

  int nCpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
  long lRamMb = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024);
#if defined(__aarch64__)
  const char *szAbi = "arm64-v8a";
#elif defined(__ARM_ARCH)
  const char *szAbi = "armeabi-v7a";
#elif defined(__x86_64__)
  const char *szAbi = "x86_64";
#else
  const char *szAbi = "unknown";
#endif
  DiagWrite("cpu: %d cores, ram: %ld MB, abi: %s", nCpu, lRamMb, szAbi);
  int v = SDL_GetVersion();
  DiagWrite("SDL: %d.%d.%d", v / 1000000, (v / 1000) % 1000, v % 1000);
}

// ---------------------------------------------------------------------------
// SDL log hook: everything SDL_Log (incl. GLESCompat) goes to diag + logcat
// ---------------------------------------------------------------------------
static void DiagSDLLog(void *userdata, int category, SDL_LogPriority priority,
                       const char *message)
{
  (void)userdata;
  static const android_LogPriority pri[] = {
      ANDROID_LOG_INFO, ANDROID_LOG_DEFAULT, ANDROID_LOG_VERBOSE,
      ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN,
      ANDROID_LOG_ERROR, ANDROID_LOG_FATAL, ANDROID_LOG_FATAL};
  const int nPri = (int)(sizeof(pri) / sizeof(pri[0]));
  android_LogPriority p = (priority >= 0 && priority < nPri) ? pri[priority]
                                                            : ANDROID_LOG_INFO;
  __android_log_print(p, "NearChuckle", "%s", message);
  DiagWriteStr(message);
  DiagRotateIfNeeded();
}

// ---------------------------------------------------------------------------
// crash handler
// ---------------------------------------------------------------------------
static const int kCrashSigs[] = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTRAP};

// unwinder: without this the report is just signal+maps and the actual
// faulting module/function is unknown (phone dumps showed zero usable info)
#include <unwind.h>
#include <dlfcn.h>
#include <ucontext.h>

struct DiagBtState
{
  int fd;
  int count;
};

static _Unwind_Reason_Code DiagBtCb(struct _Unwind_Context *uc, void *data)
{
  DiagBtState *st = (DiagBtState *)data;
  if (st->count >= 48)
    return _URC_END_OF_STACK;
  uintptr_t pc = (uintptr_t)_Unwind_GetIP(uc);
  if (pc)
  {
    char line[512];
    Dl_info di;
    memset(&di, 0, sizeof(di));
    if (dladdr((const void *)pc, &di) && di.dli_fname)
    {
      uintptr_t base = (uintptr_t)di.dli_fbase;
      if (di.dli_sname && di.dli_saddr && (uintptr_t)di.dli_saddr <= pc)
        snprintf(line, sizeof(line), "#%02d pc 0x%08zx  %s (%s+0x%zx)\n",
                 st->count, (size_t)(pc - base), di.dli_fname,
                 di.dli_sname, (size_t)(pc - (uintptr_t)di.dli_saddr));
      else
        snprintf(line, sizeof(line), "#%02d pc 0x%08zx  %s\n",
                 st->count, (size_t)(pc - base), di.dli_fname);
    }
    else
      snprintf(line, sizeof(line), "#%02d pc 0x%zx\n", st->count, (size_t)pc);
    ::write(st->fd, line, strlen(line));
  }
  st->count++;
  return _URC_CONTINUE_UNWIND;
}

// fault context from the signal frame: _Unwind_Backtrace called from inside
// the handler often returns only handler frames (observed: a 1-frame
// backtrace on arm64), while ucontext keeps the EXACT faulting PC and LR -
// that is the data which identifies the crash site
static char g_szFaultCtx[768];

static void DiagSymLine(char *dst, size_t cap, const char *label, uintptr_t addr)
{
  if (!addr)
  {
    snprintf(dst, cap, "  %s unavailable\n", label);
    return;
  }
  Dl_info di;
  memset(&di, 0, sizeof(di));
  if (dladdr((const void *)addr, &di) && di.dli_fname)
  {
    uintptr_t base = (uintptr_t)di.dli_fbase;
    if (di.dli_sname && di.dli_saddr && (uintptr_t)di.dli_saddr <= addr)
      snprintf(dst, cap, "  %s 0x%08zx  %s (%s+0x%zx)\n", label,
               (size_t)(addr - base), di.dli_fname, di.dli_sname,
               (size_t)(addr - (uintptr_t)di.dli_saddr));
    else
      snprintf(dst, cap, "  %s 0x%08zx  %s\n", label, (size_t)(addr - base),
               di.dli_fname);
  }
  else
    snprintf(dst, cap, "  %s 0x%zx\n", label, (size_t)addr);
}

// stack-scan backtrace: Far Cry is built without frame pointers and
// _Unwind_Backtrace from inside a signal handler usually returns just the
// handler itself (observed on device). Instead scan the faulting stack
// window and symbolize every value that dladdr resolves to code - the real
// call chain is always in there among the spills.
static void DiagStackScan(ucontext_t *uc, int fd)
{
  uintptr_t sp = 0;
#if defined(__aarch64__)
  if (uc)
    sp = (uintptr_t)uc->uc_mcontext.sp;
#elif defined(__arm__)
  if (uc)
    sp = (uintptr_t)uc->uc_mcontext.arm_sp;
#endif
  if (!sp)
    return;
  static const char szHdr[] = "=== stack scan (resolved code addrs) ===\n";
  ::write(fd, szHdr, sizeof(szHdr) - 1);
  char line[512];
  int printed = 0;
  // ~6 KB window, 16-byte stride, max 48 resolved entries
  for (uintptr_t a = sp; a < sp + 6 * 1024 && printed < 48; a += 16)
  {
    uintptr_t v;
    memcpy(&v, (const void *)a, sizeof(v)); // stack of the faulting thread: mapped
    if (v < 0x10000 || (v & 3))
      continue;
    Dl_info di;
    memset(&di, 0, sizeof(di));
    if (!dladdr((const void *)v, &di) || !di.dli_fname)
      continue;
    uintptr_t base = (uintptr_t)di.dli_fbase;
    if (di.dli_sname && di.dli_saddr && (uintptr_t)di.dli_saddr <= v)
      snprintf(line, sizeof(line), "  *[sp+0x%03zx] 0x%08zx  %s (%s+0x%zx)\n",
               (size_t)(a - sp), (size_t)(v - base), di.dli_fname,
               di.dli_sname, (size_t)(v - (uintptr_t)di.dli_saddr));
    else
      snprintf(line, sizeof(line), "  *[sp+0x%03zx] 0x%08zx  %s\n",
               (size_t)(a - sp), (size_t)(v - base), di.dli_fname);
    ::write(fd, line, strlen(line));
    printed++;
  }
  if (!printed)
    ::write(fd, "  (no code addresses found in stack window)\n", 42);
  ::write(fd, "=== end stack scan ===\n", 24);
}

static const char *DiagSigName(int sig)
{
  switch (sig)
  {
  case SIGSEGV: return "SIGSEGV";
  case SIGABRT: return "SIGABRT";
  case SIGFPE:  return "SIGFPE";
  case SIGILL:  return "SIGILL";
  case SIGBUS:  return "SIGBUS";
  case SIGTRAP: return "SIGTRAP";
  default:      return "?";
  }
}

static void DiagCrashHandler(int sig, siginfo_t *info, void *ctx)
{
  // async-signal-safe code only (dladdr/unwind are tolerated here like in
  // most crash reporters; without them the report is useless)
  ucontext_t *uc = (ucontext_t *)ctx;
  uintptr_t pc = 0, lr = 0;
#if defined(__aarch64__)
  if (uc)
  {
    pc = (uintptr_t)uc->uc_mcontext.pc;
    lr = (uintptr_t)uc->uc_mcontext.regs[30];
  }
#elif defined(__arm__)
  if (uc)
  {
    pc = (uintptr_t)uc->uc_mcontext.arm_pc;
    lr = (uintptr_t)uc->uc_mcontext.arm_lr;
  }
#endif
  int fd = ::open(g_szDiagPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd >= 0)
  {
    // signal info goes at the head AND at the tail: users paste the report
    // from phones where the head of a long dump gets cut off, the tail
    // always survives, so the backtrace must be at the very end
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
                     "\n=== CRASH: %s (%d), fault addr %p, si_code %d ===\n",
                     DiagSigName(sig), sig, info ? info->si_addr : 0,
                     info ? info->si_code : 0);
    ::write(fd, buf, (size_t)n);
    if (pc)
    {
      char line1[320], line2[320];
      DiagSymLine(line1, sizeof(line1), "fault pc", pc);
      DiagSymLine(line2, sizeof(line2), "lr", lr);
      n = snprintf(g_szFaultCtx, sizeof(g_szFaultCtx),
                   "=== fault context ===\n%s%s", line1, line2);
      ::write(fd, g_szFaultCtx, (size_t)n);
    }
    else
      g_szFaultCtx[0] = 0;
    // tail of /proc/self/maps: lets the dev map fault addresses to modules
    int mfd = ::open("/proc/self/maps", O_RDONLY);
    if (mfd >= 0)
    {
      static char line[256];
      // simple tail: last 250 lines via ring buffer of offsets is overkill;
      // just append everything after the marker (maps ~ tens of KB)
      ::write(fd, "=== maps ===\n", 13);
      ssize_t r;
      while ((r = ::read(mfd, line, sizeof(line))) > 0)
        ::write(fd, line, (size_t)r);
      ::close(mfd);
      ::write(fd, "=== end maps ===\n", 17);
    }
    // backtrace LAST so it always survives copy/paste truncation
    ::write(fd, "=== backtrace ===\n", 18);
    DiagBtState st;
    st.fd = fd;
    st.count = 0;
    _Unwind_Backtrace(&DiagBtCb, &st);
    n = snprintf(buf, sizeof(buf), "=== end backtrace (%d frames) ===\n", st.count);
    ::write(fd, buf, (size_t)n);
    DiagStackScan(uc, fd);
    // and once more at the very bottom - the tail of the paste is what
    // always arrives
    if (g_szFaultCtx[0])
    {
      ::write(fd, "\n", 1);
      ::write(fd, g_szFaultCtx, strlen(g_szFaultCtx));
    }
    ::close(fd);
    // marker for auto-share on next launch
    char szFlag[1100];
    snprintf(szFlag, sizeof(szFlag), "%s/crash.flag", g_szFilesDir);
    int mk = ::open(szFlag, O_WRONLY | O_CREAT, 0644);
    if (mk >= 0)
      ::close(mk);
  }
  // restore default and re-raise so the system crash dialog stays honest
  signal(sig, SIG_DFL);
  raise(sig);
}

static void InstallCrashHandler()
{
  if (g_bCrashHandlerInstalled)
    return;
  g_bCrashHandlerInstalled = true;
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = DiagCrashHandler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  for (size_t i = 0; i < sizeof(kCrashSigs) / sizeof(kCrashSigs[0]); i++)
    sigaction(kCrashSigs[i], &sa, 0);
}

// ---------------------------------------------------------------------------
// Java bridge: share + save to Downloads (MainActivity helpers)
// ---------------------------------------------------------------------------
static JNIEnv *GetEnv(jobject &outActivity, jclass &outCls)
{
  outActivity = 0;
  outCls = 0;
  JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
  jobject activity = (jobject)SDL_GetAndroidActivity();
  if (!env || !activity)
  {
    if (activity)
      env->DeleteLocalRef(activity);
    return 0;
  }
  outActivity = activity;
  outCls = env->GetObjectClass(activity);
  return env;
}

// void method(String)
static bool CallJavaVoid1(const char *szMethod, const char *szSig, const char *a)
{
  jobject activity;
  jclass cls;
  JNIEnv *env = GetEnv(activity, cls);
  if (!env)
    return false;
  bool ok = false;
  if (cls)
  {
    jmethodID mid = env->GetMethodID(cls, szMethod, szSig);
    if (mid)
    {
      jstring ja = a ? env->NewStringUTF(a) : 0;
      env->CallVoidMethod(activity, mid, ja);
      if (ja)
        env->DeleteLocalRef(ja);
      ok = !env->ExceptionCheck();
      if (env->ExceptionCheck())
        env->ExceptionClear();
    }
    env->DeleteLocalRef(cls);
  }
  env->DeleteLocalRef(activity);
  return ok;
}

// void method(String, String)
static bool CallJavaVoid2(const char *szMethod, const char *szSig,
                          const char *a, const char *b)
{
  jobject activity;
  jclass cls;
  JNIEnv *env = GetEnv(activity, cls);
  if (!env)
    return false;
  bool ok = false;
  if (cls)
  {
    jmethodID mid = env->GetMethodID(cls, szMethod, szSig);
    if (mid)
    {
      jstring ja = a ? env->NewStringUTF(a) : 0;
      jstring jb = b ? env->NewStringUTF(b) : 0;
      env->CallVoidMethod(activity, mid, ja, jb);
      if (ja)
        env->DeleteLocalRef(ja);
      if (jb)
        env->DeleteLocalRef(jb);
      ok = !env->ExceptionCheck();
      if (env->ExceptionCheck())
        env->ExceptionClear();
    }
    env->DeleteLocalRef(cls);
  }
  env->DeleteLocalRef(activity);
  return ok;
}

// String method(String, String) - returns the Java string (or NULL) and
// clears any pending exception; result must be released with ReleaseStr
static jstring CallJavaStr2Ret(const char *szMethod, const char *szSig,
                               const char *a, const char *b)
{
  jobject activity;
  jclass cls;
  JNIEnv *env = GetEnv(activity, cls);
  if (!env || !cls)
  {
    if (env)
      env->DeleteLocalRef(activity);
    return NULL;
  }
  jstring ret = NULL;
  jmethodID mid = env->GetMethodID(cls, szMethod, szSig);
  if (mid)
  {
    jstring ja = a ? env->NewStringUTF(a) : 0;
    jstring jb = b ? env->NewStringUTF(b) : 0;
    jobject res = env->CallObjectMethod(activity, mid, ja, jb);
    if (res && !env->ExceptionCheck())
      ret = (jstring)res;
    else if (res)
      env->DeleteLocalRef(res);
    if (ja)
      env->DeleteLocalRef(ja);
    if (jb)
      env->DeleteLocalRef(jb);
    if (env->ExceptionCheck())
      env->ExceptionClear();
  }
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(activity);
  return ret;
}

// copies a Java string into a fixed buffer
static void JStrToBuf(JNIEnv *env, jstring js, char *out, size_t cap)
{
  out[0] = 0;
  if (!js)
    return;
  const char *p = env->GetStringUTFChars(js, NULL);
  if (p)
  {
    snprintf(out, cap, "%s", p);
    env->ReleaseStringUTFChars(js, p);
  }
}

// ---------------------------------------------------------------------------
// combined log collection: diag.txt + engine log.txt tails
// ---------------------------------------------------------------------------
static size_t FileTail(const char *szPath, char *out, size_t maxOut)
{
  out[0] = 0;
  FILE *f = fopen(szPath, "rb");
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  long skip = sz > (long)maxOut ? sz - (long)maxOut : 0;
  fseek(f, skip, SEEK_SET);
  size_t n = fread(out, 1, maxOut - 1, f);
  out[n] = 0;
  fclose(f);
  return n;
}

static char g_szCombined[DIAG_MAX_COLLECT + 64 * 1024];
static char g_szDiagTail[DIAG_MAX_COLLECT];

// copy "=== CRASH: ... ===" blocks from the diag tail, eliding the bulky
// maps section (the backtrace is what matters); blocks are appended LAST
// because phone pastes reliably keep the end of the report and cut the head
static size_t AppendCrashBlocks(const char *src, char *dst, size_t cap)
{
  size_t o = 0;
  const char *p = src;
  while (cap > 16 && (p = strstr(p, "=== CRASH:")) != NULL)
  {
    const char *start = p;
    const char *end;
    const char *q = strstr(start, "=== end backtrace");
    if (q)
    {
      end = strchr(q, '\n');
      if (!end)
        end = start + strlen(start);
      else
        end++;
    }
    else
    {
      // unterminated block (crash mid-write): take up to 2 KB
      end = start + strlen(start);
      if ((size_t)(end - start) > 2048)
        end = start + 2048;
    }
    // elide "=== maps ===" .. "=== end maps ===" (keeps fault addr context)
    const char *m1 = strstr(start, "=== maps ===");
    const char *m2 = strstr(start, "=== end maps ===");
    if (m1 && m2 && m1 < end && m2 < end)
    {
      size_t n1 = (size_t)(m1 - start);
      if (n1 > cap - o - 1) n1 = cap - o - 1;
      memcpy(dst + o, start, n1); o += n1; dst[o] = 0;
      const char *after = m2 + strlen("=== end maps ===");
      size_t n2 = (size_t)(end - after);
      if (n2 > cap - o - 1) n2 = cap - o - 1;
      memcpy(dst + o, "[maps elided]\n", 14); o += 14;
      memcpy(dst + o, after, n2); o += n2; dst[o] = 0;
    }
    else
    {
      size_t len = (size_t)(end - start);
      if (len > cap - o - 1) len = cap - o - 1;
      memcpy(dst + o, start, len); o += len; dst[o] = 0;
    }
    p = end;
  }
  return o;
}

extern "C" const char *AndroidCollectLog()
{
  g_szCombined[0] = 0;
  size_t o = 0;
  const size_t cap = sizeof(g_szCombined) - 64;

  // engine log tail first (16 KB keeps the whole report small enough that
  // nothing gets truncated when the user forwards it)
  char szEngPath[1100];
  snprintf(szEngPath, sizeof(szEngPath), "%s/log.txt", g_szFilesDir);

  FileTail(g_szDiagPath, g_szDiagTail, sizeof(g_szDiagTail));

  o += (size_t)snprintf(g_szCombined + o, cap - o,
                        "======= NearChuckle log (user report) =======\n");
  o += (size_t)snprintf(g_szCombined + o, cap - o,
                        "\n======= engine log.txt (tail) =======\n");
  o += FileTail(szEngPath, g_szCombined + o, 16 * 1024);

  // crash backtraces go at the very END: pastes keep the tail
  o += (size_t)snprintf(g_szCombined + o, cap - o,
                        "\n======= crash report =======\n");
  size_t nb = AppendCrashBlocks(g_szDiagTail, g_szCombined + o, cap - o);
  if (nb == 0)
    o += (size_t)snprintf(g_szCombined + o, cap - o,
                          "(no crash signal was caught this run)\n");
  else
    o += nb;

  // last 3 KB of raw diag (GLESCompat spam, watchdog/rotation marks)
  {
    size_t len = strlen(g_szDiagTail);
    size_t keep = len > 3072 ? 3072 : len;
    o += (size_t)snprintf(g_szCombined + o, cap - o,
                          "\n======= diag.txt (tail) =======\n");
    const char *tail = g_szDiagTail + (len - keep);
    size_t n = (size_t)snprintf(g_szCombined + o, cap - o, "%s", tail);
    o += n;
  }
  // copy/paste completeness check: the report is only complete if this
  // exact line is present at the very end of what was sent
  o += (size_t)snprintf(g_szCombined + o, cap - o,
                        "\n======= END OF REPORT =======\n");
  return g_szCombined;
}

// share + save; bAuto=true when fired automatically after a crash
static void DoSendLogs(bool bAuto)
{
  const char *szLog = AndroidCollectLog();
  const char *szPrefix = bAuto ? "[AUTO after crash] NearChuckle log" : "NearChuckle log";
  // best effort: public copy in Downloads (visible in any file manager);
  // saveLog returns the MediaStore URI of the saved file
  jstring jsUri = CallJavaStr2Ret(
      "saveLog", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
      "nearchuckle_log.txt", szLog);
  jobject activity;
  jclass cls;
  JNIEnv *env = GetEnv(activity, cls);
  char szUri[512] = "";
  if (env)
  {
    JStrToBuf(env, jsUri, szUri, sizeof(szUri));
    if (jsUri)
      env->DeleteLocalRef(jsUri);
    if (cls)
      env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
  }
  if (szUri[0])
  {
    // share the FILE: long pasted text gets truncated by viewers and
    // messengers, a file attachment survives intact
    CallJavaVoid2("offerLogFile", "(Ljava/lang/String;Ljava/lang/String;)V",
                  szUri, szPrefix);
  }
  else
  {
    // fallback (pre-API29 devices): text share like before
    char title[160];
    snprintf(title, sizeof(title), "%s\n%s", szPrefix, szLog);
    CallJavaVoid1("offerLogShare", "(Ljava/lang/String;)V", title);
  }
}

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------
extern "C" void AndroidSendLogs(void)
{
  DoSendLogs(false);
}

void DiagInit()
{
  const char *szStorage = SDL_GetAndroidExternalStoragePath();
  if (!szStorage || !szStorage[0])
    szStorage = SDL_GetAndroidInternalStoragePath();
  if (!szStorage || !szStorage[0])
    szStorage = "/data/local/tmp";
  snprintf(g_szFilesDir, sizeof(g_szFilesDir), "%s", szStorage);
  snprintf(g_szDiagPath, sizeof(g_szDiagPath), "%s/diag.txt", g_szFilesDir);

  if (g_diagFd < 0)
    g_diagFd = ::open(g_szDiagPath, O_WRONLY | O_CREAT | O_APPEND, 0644);

  // SDL log interception (renderer, bootstrap, engine SDL_Log calls)
  SDL_LogPriority min = SDL_LogPriority(SDL_LOG_PRIORITY_VERBOSE);
  SDL_SetLogPriorities(min);
  SDL_SetLogOutputFunction(DiagSDLLog, 0);

  InstallCrashHandler();
  WriteDeviceInfo();

  // previous run crashed? offer to send the log right away
  if (!g_bAutoShareChecked)
  {
    g_bAutoShareChecked = true;
    char szFlag[1100];
    snprintf(szFlag, sizeof(szFlag), "%s/crash.flag", g_szFilesDir);
    FILE *f = fopen(szFlag, "rb");
    if (f)
    {
      fclose(f);
      unlink(szFlag);
      DiagWriteStr("previous run crashed - offering log share\n");
      DoSendLogs(true);
    }
  }
}

#else // !__ANDROID__

#include "SDL3/SDL.h"
void DiagInit() {}
extern "C" void AndroidSendLogs(void) { (void)0; }
extern "C" const char *AndroidCollectLog() { return "log collection is Android-only"; }

#endif // __ANDROID__
