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
  (void)ctx;
  // async-signal-safe code only (dladdr/unwind are tolerated here like in
  // most crash reporters; without them the report is useless)
  int fd = ::open(g_szDiagPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd >= 0)
  {
    // signal info goes at the head AND at the tail: users paste the report
    // from phones where the head of a long dump gets cut off, the tail
    // always survives, so the backtrace must be at the very end
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
                     "\n=== CRASH: %s (%d), fault addr %p ===\n",
                     DiagSigName(sig), sig, info ? info->si_addr : 0);
    ::write(fd, buf, (size_t)n);
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

// String method(String, String)
static bool CallJavaStr2(const char *szMethod, const char *szSig, const char *a, const char *b)
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
      env->CallObjectMethod(activity, mid, ja, jb);
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

static char g_szCombined[DIAG_MAX_COLLECT + DIAG_MAX_COLLECT + 4096];

extern "C" const char *AndroidCollectLog()
{
  g_szCombined[0] = 0;
  size_t o = 0;
  const size_t cap = sizeof(g_szCombined) - 64;

  o += (size_t)snprintf(g_szCombined + o, cap - o,
                        "======= NearChuckle log (user report) =======\n");
  o += FileTail(g_szDiagPath, g_szCombined + o, DIAG_MAX_COLLECT);
  o += (size_t)snprintf(g_szCombined + o, cap - o,
                        "\n======= engine log.txt =======\n");
  char szEng[1100];
  snprintf(szEng, sizeof(szEng), "%s/log.txt", g_szFilesDir);
  o += FileTail(szEng, g_szCombined + o, DIAG_MAX_COLLECT);
  if (!strstr(g_szCombined, "NearChuckle run started"))
  {
    o += (size_t)snprintf(g_szCombined + o, cap - o, "(diag log empty)\n");
  }
  return g_szCombined;
}

// share + save; bAuto=true when fired automatically after a crash
static void DoSendLogs(bool bAuto)
{
  const char *szLog = AndroidCollectLog();
  char title[160];
  snprintf(title, sizeof(title), "%s\n%s", bAuto ? "[AUTO after crash]" : "NearChuckle log",
           szLog);
  // best effort: public copy in Downloads (visible in any file manager)
  CallJavaStr2("saveLog", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
               "nearchuckle_log.txt", title);
  // share sheet: user sends it to any chat/email directly from the phone
  CallJavaVoid1("offerLogShare", "(Ljava/lang/String;)V", title);
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
