#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
OUT=/tmp/pa220-load-range-host-test
cc -std=c99 -Wall -Wextra -Werror -O2 \
  -DPA220_LOAD_RANGE_HOST_TEST \
  -I"$ROOT/tests/pa220/ram-range" \
  -I"$ROOT/u-boot/arch/mips/include/asm/arch-octeon" \
  "$ROOT/u-boot/arch/mips/cpu/octeon/octeon_load_range.c" \
  "$ROOT/tests/pa220/ram-range/host_test.c" \
  -o "$OUT"
"$OUT"
