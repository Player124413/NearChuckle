#!/usr/bin/env bash
#
# Build SwiftShader (software EGL/GLES) for the headless host smoke test.
#
# The host smoke test runs the game's renderer on the SAME code path as the
# Android build: GFX_GLES3 + GLESCompat over an OpenGL ES context. On a PC
# without a GPU/display we use SwiftShader's software EGL. These steps are
# only needed on a developer machine / CI runner, never on the phone.
#
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GLSTUBS="${GLSTUBS:-$HOME/glestubs}"

# 1. SDL patch: let SDL's 'offscreen' video driver use the default EGL
#    display when the driver lacks EGL_EXT_device_enumeration (SwiftShader).
echo "==> Applying SDL offscreen+software-EGL patch"
(cd "$ROOT/Externals/SDL" && git apply --check "$ROOT/ci-tools/sdl-offscreen-softegl.patch" 2>/dev/null \
  && git apply "$ROOT/ci-tools/sdl-offscreen-softegl.patch") || echo "  (already applied)"

if [ -f "$GLSTUBS/libEGL.so" ] && [ -f "$GLSTUBS/libGLESv2.so" ]; then
  echo "SwiftShader already present in $GLSTUBS"
  exit 0
fi

# 2. SwiftShader: the last branch that still ships libEGL/libGLESv2.
echo "==> Cloning SwiftShader (legacy-gles1)"
git clone --depth 1 -b legacy-gles1 https://github.com/google/swiftshader /tmp/ss-gles

# 3. Minimal X11 headers (SwiftShader compiles an X11 framebuffer backend we
#    don't use; provide just enough types to compile).
mkdir -p /tmp/x11stub/X11/extensions
if [ -d "$ROOT/ci-tools/x11-stub-headers/X11" ]; then
  cp -r "$ROOT/ci-tools/x11-stub-headers/X11/." /tmp/x11stub/X11/
fi

echo "==> Building SwiftShader (libEGL + libGLESv2 only)"
cmake -S /tmp/ss-gles -B /tmp/ss-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-I/tmp/x11stub" \
  -DSWIFTSHADER_BUILD_VULKAN=OFF \
  -DSWIFTSHADER_BUILD_GLES_CM=OFF \
  -DSWIFTSHADER_BUILD_SAMPLES=OFF \
  -DSWIFTSHADER_BUILD_TESTS=OFF \
  -DSWIFTSHADER_BUILD_PVR=OFF \
  -DSWIFTSHADER_WARNINGS_AS_ERRORS=OFF \
  -DSWIFTSHADER_ENABLE_ASSERT=OFF
ninja -C /tmp/ss-build libEGL libGLESv2

mkdir -p "$GLSTUBS"
cp /tmp/ss-build/libEGL.so /tmp/ss-build/libGLESv2.so "$GLSTUBS/"
echo "==> SwiftShader installed to $GLSTUBS"
