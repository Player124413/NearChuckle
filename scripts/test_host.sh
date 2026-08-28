#!/usr/bin/env bash
#
# Host build + unit tests (validates engine changes without an NDK).
#
set -euo pipefail
cd "$(dirname "$0")/.."

CMAKE_BIN="${CMAKE_BIN:-cmake}"
echo "==> Configuring host build"
"$CMAKE_BIN" -S . -B build/host -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DDISABLE_CG=ON \
  -DSDL_UNIX_CONSOLE_BUILD=ON

echo "==> Building"
"$CMAKE_BIN" --build build/host --parallel "${JOBS:-$(nproc)}"

echo "==> Unit tests"
g++ -std=c++17 -DTOUCHJSON_STANDALONE tests/TouchJsonTest.cpp -o /tmp/touchtest
/tmp/touchtest
g++ -std=c++17 tests/TouchLogicTest.cpp -o /tmp/touchlogic
/tmp/touchlogic

echo "==> Smoke test (boot with NULL renderer, no game data expected)"
BIN=$(ls -d bin/x64-Release 2>/dev/null || ls -d bin 2>/dev/null | head -1)
cd "$BIN"
printf 'r_Driver = "NULL"\nr_Width = 640\nr_Height = 480\n' > system.cfg
timeout 20 ./FarCry || true
if grep -q "Touch device initialized" ../log.txt 2>/dev/null || grep -q "Touch device initialized" log.txt 2>/dev/null; then
  echo "==> Smoke test OK: touch device came up"
else
  echo "!! touch device did not initialize - check log" >&2
  tail -30 ../log.txt 2>/dev/null || tail -30 log.txt 2>/dev/null || true
  exit 1
fi
rm -f system.cfg log.txt ../log.txt
echo "==> ALL HOST CHECKS PASSED"
