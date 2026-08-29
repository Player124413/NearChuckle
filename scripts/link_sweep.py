#!/usr/bin/env python3
"""Linker sanity sweep over the host-built engine modules.

DEVICE-STRICT mode: on Android, dlopen resolves a module's undefined symbols
ONLY against the module's own DT_NEEDED chain (loaded with RTLD_LOCAL; the
host-side RTLD_GLOBAL pool of engine modules does not exist there - bug #3:
libCryNetwork/libCrySoundSystem referenced SDL_GetTicks without a DT_NEEDED
libSDL3.so and failed to load on the phone).

So for every bin lib we verify that every undefined symbol resolves inside:
  (a) the transitive closure of its own DT_NEEDED libraries (project deps in
      the same bin dir), or
  (b) the system/base symbol pool (host glibc/GL + Android base libs).
Anything else is reported as a device dlopen failure.
"""
import os, re, glob, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "bin/x64-Release")
SKIP_PREFIXES = ("libSDL3", "libopenal", "libvorbisfile", "libvorbis", "libogg")

def nm(f, mode):
    out = subprocess.run(["nm", "-D", mode, f], capture_output=True, text=True).stdout
    hard, weak, undef = set(), set(), set()
    for l in out.splitlines():
        p = l.split()
        if not p:
            continue
        # undefined symbols print as "U name" (2 fields) - they were silently
        # dropped by an older 3-field parser, making the sweep vacuous!
        if p[0] == "U" and len(p) == 2:
            undef.add(re.sub(r"@.*", "", p[1]))
            continue
        if len(p) >= 3 and p[1] == "U":
            undef.add(re.sub(r"@.*", "", p[-1]))
            continue
        if len(p) >= 3:
            name = re.sub(r"@.*", "", p[-1])
            (weak if p[1] in ('w', 'v') else hard).add(name)  # only w/v optional; 'i' (IFUNC) = real export
    return hard, weak, undef

def needed(f):
    out = subprocess.run(["readelf", "-d", f], capture_output=True, text=True).stdout
    return [os.path.basename(m) for m in re.findall(r"\(NEEDED\).*\[(.+)\]", out)]

libs = sorted(glob.glob(os.path.join(BIN, "lib*.so*")))
if not libs:
    print(f"link_sweep: no libs found in {BIN}", file=sys.stderr)
    sys.exit(2)

exports, undefs, weaks, needs = {}, {}, {}, {}
for so in libs:
    n = os.path.basename(so)
    exports[n], _, _ = nm(so, "--defined-only")
    _, weaks[n], undefs[n] = nm(so, "--undefined-only")
    needs[n] = needed(so)

# system symbols: everything the host system libs export (glibc/libstdc++/GL/...)
sys_syms = set()
for pat in ("/lib/x86_64-linux-gnu/lib*.so.6", "/usr/lib/x86_64-linux-gnu/lib*.so.6",
            "/usr/lib/x86_64-linux-gnu/libGL.so.1", "/usr/lib/x86_64-linux-gnu/libX11.so.6"):
    for f in glob.glob(pat):
        try:
            sys_syms |= nm(f, "--defined-only")[0]
        except Exception:
            pass
sys_syms |= {"__android_log_print", "__android_log_write", "__android_log_vprint",
             "__android_log_buf_print", "__android_log_is_loggable"}
# compiler/runtime intrinsics emitted into every ELF, resolved by the Android
# runtime (libunwind/compiler-rt/bionic) or simply ignored by the loader
RUNTIME_RE = re.compile(r"^(_ITM_|__gmon_start__|_Unwind_|__gcc_personality|__tls_get_addr"
                        r"|__popcount|__clz|__ctz|__bswap|__aeabi|__udivti3|__divti3|__umodti3"
                        r"|__modti3|__udivmodti4|__emutls|__gnu_|_+(mul|div|sub|add)[sdt]?c3$|_edata$|_end$|_fini$|_init$)")

fail = 0
for n in sorted(undefs):
    if any(n.startswith(p) for p in SKIP_PREFIXES):
        continue
    # transitive DT_NEEDED closure within the bin dir (what the Android loader
    # will actually have available when resolving this module's imports)
    allowed, stack, visited = set(), [d for d in needs[n]], set()
    while stack:
        cur = stack.pop()
        if cur in visited:
            continue
        visited.add(cur)
        allowed |= exports.get(cur, set())
        stack.extend(d for d in needs.get(cur, []) if d not in visited)
    missing = undefs[n] - allowed - sys_syms
    missing = {s for s in missing if not RUNTIME_RE.match(s)}
    hard_missing = {s for s in missing if s not in weaks[n]}
    if hard_missing:
        fail += 1
        print(f"{n}: {len(hard_missing)} symbols would NOT resolve on device "
              f"(own DT_NEEDED only):")
        for s in sorted(hard_missing)[:20]:
            print(f"    {s}")
        if len(hard_missing) > 20:
            print(f"    ... and {len(hard_missing)-20} more")
if not fail:
    print("LINK SWEEP OK: every module's imports resolve via its own DT_NEEDED "
          "chain (device-strict)")
sys.exit(1 if fail else 0)
