#!/usr/bin/env python3
"""Structural sanity check for the Android Java sources.

Catches the 'cut a method signature together with a removed block' class of
breakage (CI once died with 100 javac errors after an edit left method
bodies outside their class): verifies bracket balance with string/char/
comment awareness and a few coarse structure rules. No JDK needed.
"""
import glob, sys

def check(path):
    s = open(path, encoding="utf-8").read()
    line = 1; i = 0; n = len(s)
    state = "code"  # code | line_comment | block_comment | str | char
    stack = []      # open-brace lines
    problems = []
    while i < n:
        c = s[i]; nxt = s[i+1] if i + 1 < n else ""
        if c == "\n":
            line += 1
        if state == "code":
            if c == "/" and nxt == "/":
                state = "line_comment"; i += 2; continue
            if c == "/" and nxt == "*":
                state = "block_comment"; i += 2; continue
            if c == '"':
                state = "str"
            elif c == "'":
                state = "char"
            elif c == "{":
                stack.append(line)
            elif c == "}":
                if not stack:
                    problems.append(f"{path}:{line}: unmatched '}}'")
                else:
                    stack.pop()
        elif state == "line_comment":
            if c == "\n":
                state = "code"
        elif state == "block_comment":
            if c == "*" and nxt == "/":
                state = "code"; i += 2; continue
        elif state == "str":
            if c == "\\":
                i += 2; continue
            if c == '"':
                state = "code"
        elif state == "char":
            if c == "\\":
                i += 2; continue
            if c == "'":
                state = "code"
        i += 1
    if stack:
        problems.append(f"{path}: unclosed '{{' opened at line(s) {stack}")
    if state == "block_comment":
        problems.append(f"{path}: unterminated block comment")
    if not s.lstrip().startswith("package "):
        problems.append(f"{path}: missing package declaration")
    return problems

files = sorted(glob.glob("android/app/src/main/java/**/*.java", recursive=True))
if not files:
    print("javacheck: no java files found", file=sys.stderr)
    sys.exit(2)

fail = 0
for f in files:
    for p in check(f):
        print(p)
        fail += 1
if fail:
    print(f"JAVA CHECK FAILED: {fail} problem(s) in {len(files)} files")
else:
    print(f"JAVA CHECK OK: {len(files)} files structurally sound")
sys.exit(1 if fail else 0)
