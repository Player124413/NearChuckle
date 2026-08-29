#!/usr/bin/env bash
#
# Host build + unit tests (validates engine changes without an NDK).
#
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$PWD"

# Headless host testing: GFX_GLES3 needs an ES context; SDL's offscreen
# driver + SwiftShader software EGL provide it without X11/GPU. The SDL
# patch (default-display fallback) must be in place before configuring.
bash "$ROOT/scripts/build_swiftshader.sh" || echo "!! SwiftShader setup failed - smoke will try desktop GL" >&2

CMAKE_BIN="${CMAKE_BIN:-cmake}"
echo "==> Configuring host build"
"$CMAKE_BIN" -S . -B build/host -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DDISABLE_CG=ON \
  -DSDL_UNIX_CONSOLE_BUILD=ON

echo "==> Building"
"$CMAKE_BIN" --build build/host --parallel "${JOBS:-$(nproc)}"

echo "==> Link sweep (undefined symbols that would not resolve on device)"
python3 "$ROOT/scripts/link_sweep.py" "$ROOT/bin/x64-Release"

echo "==> Unit tests"
g++ -std=c++17 -DTOUCHJSON_STANDALONE tests/TouchJsonTest.cpp -o /tmp/touchtest
/tmp/touchtest
g++ -std=c++17 tests/TouchLogicTest.cpp -o /tmp/touchlogic
/tmp/touchlogic

echo "==> Smoke test (boot with GLESCompat renderer, no game data expected)"
BIN=$(ls -d bin/x64-Release 2>/dev/null || ls -d bin 2>/dev/null | head -1)
cd "$BIN"
printf 'r_Driver = "NULL"\nr_Width = 640\nr_Height = 480\n' > system.cfg

# The host build uses the same renderer path as the phone (GFX_GLES3:
# GLESCompat over an OpenGL ES context). Without a GPU/display we run it on
# SwiftShader's software EGL through SDL's offscreen video driver.
GLSTUBS="${GLSTUBS:-$HOME/glestubs}"
if [ ! -f "$GLSTUBS/libEGL.so" ]; then
  bash "$ROOT/scripts/build_swiftshader.sh" || true
fi
if [ -f "$GLSTUBS/libEGL.so" ]; then
  echo "  (headless mode: SwiftShader EGL from $GLSTUBS)"
  RUNNER="env -u DISPLAY SDL_VIDEODRIVER=offscreen SDL_EGL_LIBRARY=$GLSTUBS/libEGL.so SDL_OPENGL_LIBRARY=$GLSTUBS/libGLESv2.so timeout 60 ./FarCry"
else
  echo "  (desktop GL mode)"
  RUNNER="timeout 20 ./FarCry"
fi
sh -c "$RUNNER" || true
if grep -q "Touch device initialized" ../log.txt 2>/dev/null || grep -q "Touch device initialized" log.txt 2>/dev/null; then
  echo "==> Smoke test OK: touch device came up"
else
  echo "!! touch device did not initialize - check log" >&2
  tail -30 ../log.txt 2>/dev/null || tail -30 log.txt 2>/dev/null || true
  exit 1
fi
rm -f system.cfg log.txt ../log.txt
echo "==> ALL HOST CHECKS PASSED"
