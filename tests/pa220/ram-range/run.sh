#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
OUT=/tmp/pa220-load-range-host-test
COMMON="-DPA220_LOAD_RANGE_HOST_TEST -I$ROOT/tests/pa220/ram-range -I$ROOT/u-boot/arch/mips/include/asm/arch-octeon"
SRC="$ROOT/u-boot/arch/mips/cpu/octeon/octeon_load_range.c $ROOT/tests/pa220/ram-range/host_test.c"
cc -std=c99 -Wall -Wextra -Werror -O2 $COMMON $SRC -o "$OUT"
"$OUT"
cc -std=c99 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer $COMMON $SRC -o "$OUT-sanitized"
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 "$OUT-sanitized"
