#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
OUT="${TMPDIR:-/tmp}/pa220-http-request-test"
CC="${CC:-cc}"
mkdir -p "$OUT"
"$CC" -std=c89 -Wall -Wextra -Werror -pedantic -I"$ROOT/tests/host/http_request/stubs" -I"$ROOT/u-boot/include" "$ROOT/u-boot/net/http_request.c" "$ROOT/tests/host/http_request/test.c" -o "$OUT/http_request_test"
"$OUT/http_request_test"
