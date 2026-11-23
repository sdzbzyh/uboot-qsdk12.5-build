#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
OUT="${TMPDIR:-/tmp}/pa220-http-handler-test"
CC="${CC:-cc}"
mkdir -p "$OUT"
"$CC" -std=gnu89 -Wall -Wextra -Werror \
 -I"$ROOT/tests/host/http_handler/stubs" \
 -I"$ROOT/tests/host/http_request/stubs" \
 -I"$ROOT/u-boot/include" \
 "$ROOT/u-boot/net/http_request.c" \
 "$ROOT/u-boot/net/http_handler.c" \
 "$ROOT/u-boot/net/http_handler_uboot.c" \
 "$ROOT/tests/host/http_handler/test.c" -o "$OUT/http_handler_test"
"$OUT/http_handler_test"
