#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
OUT=/tmp/pa220-tcp-ipv4-tx-test
cc -std=c99 -Wall -Wextra -Werror \
  -I"$ROOT/tests/host/tcp_ipv4_tx/stubs" \
  -I"$ROOT/u-boot/include" \
  "$ROOT/u-boot/net/tcp_ipv4_tx.c" \
  "$ROOT/tests/host/tcp_ipv4_tx/test.c" -o "$OUT"
"$OUT"
