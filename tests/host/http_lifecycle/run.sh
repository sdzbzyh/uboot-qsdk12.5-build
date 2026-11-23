#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
TEST_DIR="$ROOT/tests/host/http_lifecycle"
BUILD_DIR=$(mktemp -d "${TMPDIR:-/tmp}/pa220-p5-2c-host.XXXXXX")
trap 'rm -rf "$BUILD_DIR"' EXIT
CC=${CC:-cc}
"$CC" -std=gnu99 -Wall -Wextra -fsyntax-only \
  -I"$TEST_DIR/include" -I"$ROOT/u-boot/include" \
  "$ROOT/u-boot/common/cmd_pa220_web.c" \
  "$ROOT/u-boot/net/http_handler_uboot.c"
"$CC" --version | head -n 1
"$CC" -std=c99 -O2 -g -Wall -Wextra \
  -I"$TEST_DIR/include" -I"$ROOT/u-boot/include" \
  "$TEST_DIR/test_http_lifecycle.c" \
  "$ROOT/u-boot/net/tcp.c" \
  "$ROOT/u-boot/net/http_request.c" \
  "$ROOT/u-boot/net/http_handler.c" \
  -o "$BUILD_DIR/test_http_lifecycle"

"$CC" -std=gnu99 -O2 -g -Wall -Wextra \
  -I"$TEST_DIR/include" -I"$ROOT/u-boot/include" \
  "$TEST_DIR/test_foreground.c" \
  "$ROOT/u-boot/common/cmd_pa220_web.c" \
  "$ROOT/u-boot/net/tcp.c" \
  "$ROOT/u-boot/net/http_request.c" \
  "$ROOT/u-boot/net/http_handler.c" \
  "$ROOT/u-boot/net/http_handler_uboot.c" \
  -o "$BUILD_DIR/test_foreground"
"$BUILD_DIR/test_foreground"
"$BUILD_DIR/test_http_lifecycle"
