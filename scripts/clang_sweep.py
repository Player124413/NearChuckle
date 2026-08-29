#!/usr/bin/env python3
"""clang frontend sweep over the NDK module set, using host build flags.

Catches gcc-vs-clang C++17 errors (parenthesized array-new, brace narrowing,
void* arithmetic, dynamic exception specs, auto_ptr, ...) locally instead of
one-per-CI-iteration. Requires: pip3 install libclang, and an existing
build/host/build.ninja (scripts/test_host.sh).
"""
import json, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
from clang import cindex

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NINJA = os.path.join(ROOT, "build/host/build.ninja")

# modules the Android NDK build compiles (FARCRY only via AndroidApp/Main.cpp)
NDK_PREFIXES = (
    "SourceCode/Cry3DEngine/", "SourceCode/CryAISystem/", "SourceCode/CryAnimation/",
    "SourceCode/CryEntitySystem/", "SourceCode/CryFont/", "SourceCode/CryGame/",
    "SourceCode/CryInput/", "SourceCode/CryMovie/", "SourceCode/CryNetwork/",
    "SourceCode/CryPhysics/", "SourceCode/CryScriptSystem/", "SourceCode/CrySoundSystem/",
    "SourceCode/CrySystem/", "SourceCode/RenderDll/XRenderOGL/", "SourceCode/RenderDll/XRenderNULL/",
    "SourceCode/RenderDll/Common/",
    "SourceCode/AndroidApp/", "SourceCode/FARCRY/Main.cpp",
)

def parse_ninja():
    src = open(NINJA, encoding="utf-8", errors="replace").read()
    units = {}
    cur = None
    for line in src.splitlines():
        if line.startswith("build ") and ("CXX_COMPILER__" in line or "C_COMPILER__" in line):
            parts = line.split(": ", 1)[1].split()
            rule = parts[0]
            out = line.split(":", 1)[0][len("build "):].strip()
            srcfile = None
            for tok in parts[1:]:
                if not tok.startswith("|") and not tok.startswith("||"):
                    srcfile = tok
                    break
            is_c = rule.startswith("C_COMPILER")
            cur = {"out": out, "src": srcfile, "is_c": is_c,
                   "DEFINES": "", "INCLUDES": "", "FLAGS": ""}
            units[out] = cur
        elif cur is not None and line.startswith("  "):
            k, _, v = line.strip().partition(" = ")
            if k in ("DEFINES", "INCLUDES", "FLAGS"):
                cur[k] = v
        else:
            cur = None
    return [u for u in units.values() if u["src"]]

def clang_args(u):
    args = u["FLAGS"].split() + u["DEFINES"].split() + u["INCLUDES"].split()
    args += ["-DANDROID", "-D__ANDROID__", "-DDISABLE_CG"]
    args += (["-x", "c", "-std=gnu11"] if u["is_c"] else ["-x", "c++", "-std=gnu++17"])
    # replicate NDK toolchain hardening defaults (they are -Werror there)
    args += ["-Werror=format-security", "-ferror-limit=0"]
    return args

def system_includes():
    out = subprocess.run(["gcc", "-xc++", "-E", "-v", "/dev/null"],
                         capture_output=True, text=True).stderr
    try:
        seg = out.split("#include <...> search starts here:")[1]
        paths = seg.split("End of search list.")[0].strip().splitlines()
    except IndexError:
        paths = ["/usr/include"]
    return sum([["-isystem", p.strip()] for p in paths], [])

def main():
    import clang, glob
    only = sys.argv[1] if len(sys.argv) > 1 else ""
    libs = glob.glob(os.path.join(os.path.dirname(clang.__file__), "native", "libclang*"))
    if libs:
        cindex.conf.set_library_file(libs[0])
    idx = cindex.Index.create()
    sysinc = system_includes()
    units = [u for u in parse_ninja() if u["src"] and any(p in u["src"] for p in NDK_PREFIXES)]
    if only:
        units = [u for u in units if only in u["src"]]
    print(f"sweeping {len(units)} translation units...")

    errors = {}
    def run(u):
        args = clang_args(u) + sysinc
        try:
            tu = idx.parse(u["src"], args=args,
                           options=cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES * 0)
        except Exception as e:
            return (u["src"], [(0, f"parse exception: {e}")])
        out = []
        for d in tu.diagnostics:
            # NDK clang emits some C++17 rejections ('register', ...) as errors
            # while libclang classifies the same diagnostics as warnings.
            is_err = d.severity >= 4 or (d.severity == 3 and (
                "does not allow" in d.spelling
                or "format string is not a string literal" in d.spelling
                or "cannot pass object of non-trivial type" in d.spelling
                or "call to undeclared function" in d.spelling))
            if is_err:
                f = str(d.location.file or u["src"])
                out.append((f"{f}:{d.location.line}", d.spelling.split("\n")[0][:160]))
        return (u["src"], out)

    with ThreadPoolExecutor(max_workers=2) as ex:
        for src, found in ex.map(run, units):
            for loc, msg in found:
                errors.setdefault((loc, msg), []).append(os.path.basename(src))

    if not errors:
        print("NO FRONTEND ERRORS FOUND")
        return
    print(f"{len(errors)} unique errors:")
    for (loc, msg), users in sorted(errors.items()):
        print(f"{loc}: {msg}   [in {len(users)} TU, e.g. {users[0]}]")

if __name__ == "__main__":
    main()
