#!/usr/bin/env python3
"""Linker sanity sweep over the host-built engine modules.

For every bin/<cfg>/lib*.so, verifies that its undefined symbols resolve
against: (a) the exports of its own DT_NEEDED libraries (project deps in the
same bin dir), (b) the pool of engine module exports (modules are loaded with
RTLD_GLOBAL at runtime, so cross-module symbols resolve at dlopen time), or
(c) system libraries (host glibc/GL + the Android base libs set).

Anything left over would be unresolved on the device too (e.g. SDL_* used
without linking SDL3) and is reported. Run after a host build.
"""
import os, re, glob, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "bin/x64-Release")
SKIP = {"libSDL3.so", "libopenal.so", "libvorbisfile.so", "libvorbis.so", "libogg.so"}

def nm(f, mode):
    out = subprocess.run(["nm", "-D", mode, f], capture_output=True, text=True).stdout
    hard, weak = set(), set()
    for l in out.splitlines():
        p = l.split()
        if len(p) < 3 or p[1] == "U":
            continue
        name = re.sub(r"@.*", "", p[-1])
        (weak if p[1].islower() else hard).add(name)  # 'w'/'v' = weak: optional, NULL if absent
    return hard, weak

def needed(f):
    out = subprocess.run(["readelf", "-d", f], capture_output=True, text=True).stdout
    return [os.path.basename(m) for m in re.findall(r"\(NEEDED\).*\[(.+)\]", out)]

libs = sorted(glob.glob(os.path.join(BIN, "lib*.so")))
if not libs:
    print(f"link_sweep: no libs found in {BIN}", file=sys.stderr)
    sys.exit(2)

exports, undefs, weaks, needs = {}, {}, {}, {}
for so in libs:
    n = os.path.basename(so)
    exports[n], _ = nm(so, "--defined-only")
    undefs[n], weaks[n] = nm(so, "--undefined-only")
    needs[n] = needed(so)

engine_pool = set().union(*(exports[n] for n in exports))  # RTLD_GLOBAL dlopen pool

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
# compiler/runtime intrinsics: weak-UND stubs emitted into every ELF, resolved by
# the Android runtime (libunwind/compiler-rt/bionic) or simply ignored by the loader
sys_syms |= {s for s in ("".join(undefs.values()) if False else set())}
RUNTIME_RE = re.compile(r"^(_ITM_|__gmon_start__|_Unwind_|__gcc_personality|__tls_get_addr"
                        r"|__popcount|__clz|__ctz|__bswap|__aeabi|__udivti3|__divti3|__umodti3"
                        r"|__modti3|__udivmodti4|__emutls|__gnu_|_(mul|div)[sdt]?c3$|_edata$|_end$|_fini$|_init$)")

fail = 0
for n in sorted(undefs):
    if n in SKIP:
        continue
    own = set().union(*(exports.get(d, set()) for d in needs[n]))
    missing = undefs[n] - own - engine_pool - sys_syms
    missing = {s for s in missing if not RUNTIME_RE.match(s)}
    optional = weaks[n] - own - engine_pool - sys_syms
    if missing:
        fail += 1
        print(f"{n}: {len(missing)} symbols would NOT resolve on device:")
        for s in sorted(missing)[:20]:
            print(f"    {s}")
        if len(missing) > 20:
            print(f"    ... and {len(missing)-20} more")
    elif optional:
        print(f"{n}: {len(optional)} optional (weak) symbols, NULL if absent: {sorted(optional)[:6]}")
if not fail:
    print("LINK SWEEP OK: every module's undefined symbols resolve on device")
sys.exit(1 if fail else 0)
