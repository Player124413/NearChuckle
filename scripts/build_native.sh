#!/usr/bin/env bash
#
# Builds all NearChuckle native modules for Android and places them into
# android/app/src/main/jniLibs/<abi>/ for packaging by Gradle.
#
# Usage:
#   ./scripts/build_native.sh [release|debug] [abi ...]
#
# Environment:
#   ANDROID_NDK_HOME / NDK_HOME / nttld setup-ndk output - NDK location
#   ABIS             - default: "arm64-v8a armeabi-v7a"
#
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

BUILD_TYPE="${1:-release}"
shift || true
if [ "$#" -gt 0 ]; then
  ABIS="$*"
else
  ABIS="${ABIS:-arm64-v8a armeabi-v7a}"
fi

# locate the NDK
if [ -z "${ANDROID_NDK_HOME:-}" ]; then
  for c in "${ANDROID_HOME:-/opt/ndk}" "${ANDROID_SDK_ROOT:-}" \
           /usr/local/lib/android/sdk/ndk-bundle \
           /usr/local/lib/android/sdk/ndk/*; do
    if [ -d "$c" ] && ls "$c"/build/cmake/android.toolchain.cmake >/dev/null 2>&1; then
      export ANDROID_NDK_HOME="$c"
      break
    fi
  done
fi
if [ -z "${ANDROID_NDK_HOME:-}" ] && [ -n "${NDK_HOME:-}" ]; then
  export ANDROID_NDK_HOME="$NDK_HOME"
fi
if [ -z "${ANDROID_NDK_HOME:-}" ]; then
  echo "ERROR: Android NDK not found. Set ANDROID_NDK_HOME." >&2
  exit 1
fi
echo "==> Using NDK: $ANDROID_NDK_HOME"
CMAKE_TOOLCHAIN="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake"
[ -f "$CMAKE_TOOLCHAIN" ] || { echo "ERROR: toolchain file missing at $CMAKE_TOOLCHAIN" >&2; exit 1; }

CMAKE_BIN="${CMAKE_BIN:-cmake}"
command -v "$CMAKE_BIN" >/dev/null || { echo "ERROR: cmake not found" >&2; exit 1; }

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

for ABI in $ABIS; do
  echo "==> Building ABI: $ABI ($BUILD_TYPE)"
  BUILD_DIR="$ROOT/android/build/native/$ABI"
  OUT_DIR="$ROOT/android/app/src/main/jniLibs/$ABI"
  mkdir -p "$BUILD_DIR" "$OUT_DIR"

  if [ "$BUILD_TYPE" = "debug" ]; then
    CMAKE_BUILD_TYPE=Debug
  else
    CMAKE_BUILD_TYPE=Release
  fi

  # full configure+build log goes to a file (uploaded on CI failure);
  # console keeps short tails so failures are visible at a glance
  LOG="$ROOT/android/build/native-$ABI.log"
  : > "$LOG"

  if ! "$CMAKE_BIN" -S "$ROOT" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM=android-21 \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DCMAKE_MAKE_PROGRAM="${NINJA_BIN:-ninja}" \
    -DDISABLE_CG=ON \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections" \
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--gc-sections" \
    >> "$LOG" 2>&1; then
    echo "==> CONFIGURE FAILED for $ABI (full log: $LOG):"
    tail -40 "$LOG"
    exit 1
  fi
  tail -3 "$LOG"

  if ! "$CMAKE_BIN" --build "$BUILD_DIR" --parallel "$JOBS" >> "$LOG" 2>&1; then
    echo "==> BUILD FAILED for $ABI (full log: $LOG):"
    grep -m 12 -E "error:" "$LOG" || tail -40 "$LOG"
    exit 1
  fi
  tail -4 "$LOG"

  echo "==> Collecting libs for $ABI"
  # engine modules land in bin/<arch>-<type> (per-config outputs)
  SRC_BIN_DIR="$ROOT/bin"
  count=0
  for so in "$SRC_BIN_DIR"/*.so \
            "$SRC_BIN_DIR"/*-Release/*.so "$SRC_BIN_DIR"/*-Debug/*.so \
            "$BUILD_DIR"/SourceCode/bin*/*.so "$BUILD_DIR"/bin*/*.so \
            "$BUILD_DIR"/*.so; do
    [ -e "$so" ] || continue
    base="$(basename "$so")"
    cp -f "$so" "$OUT_DIR/$base"
    count=$((count+1))
  done
  echo "    copied $count libraries to $OUT_DIR"
  # libc++_shared.so is required alongside the modules (ANDROID_STL=c++_shared)
  TRIPLE="$ABI"; [ "$ABI" = "arm64-v8a" ] && TRIPLE="aarch64-linux-android"
  [ "$ABI" = "armeabi-v7a" ] && TRIPLE="arm-linux-androideabi"
  LIBCXX="$(find "$ANDROID_NDK_HOME/toolchains/llvm/prebuilt"/*/sysroot/usr/lib/"$TRIPLE" -name libc++_shared.so 2>/dev/null | head -1 || true)"
  if [ -n "$LIBCXX" ]; then
    cp -f "$LIBCXX" "$OUT_DIR/libc++_shared.so"
    echo "    copied libc++_shared.so from NDK"
  else
    echo "    WARNING: libc++_shared.so not found in NDK sysroot for $TRIPLE" >&2
  fi
  ls -la "$OUT_DIR" | tail -n +2
  # clean for the next ABI
  rm -rf "$SRC_BIN_DIR"/*.so "$SRC_BIN_DIR"/*-Release "$SRC_BIN_DIR"/*-Debug 2>/dev/null || true
done

echo "==> Native build complete."
