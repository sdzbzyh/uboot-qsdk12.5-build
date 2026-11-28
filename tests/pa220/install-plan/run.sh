#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../../" && pwd)
OUT="$ROOT/reports/pa220/P7-3"
mkdir -p "$OUT"
CC=${CC:-gcc}
SRC="$ROOT/u-boot/board/octeon/pa220/pa220_install_plan.c"
TEST="$ROOT/tests/pa220/install-plan/test.c"
INCLUDE="$ROOT/u-boot/include"
"$CC" -DPA220_INSTALL_PLAN_HOST_TEST -std=gnu99 -Wall -Wextra -Werror -I"$INCLUDE" "$SRC" "$TEST" -o "$OUT/p7-3b-host"
"$OUT/p7-3b-host" | tee "$OUT/40-host-plan.log"
"$CC" -DPA220_INSTALL_PLAN_HOST_TEST -std=gnu99 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -g -I"$INCLUDE" "$SRC" "$TEST" -o "$OUT/p7-3b-sanitize"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$OUT/p7-3b-sanitize" | tee "$OUT/41-sanitizers.log"
grep -q 'P7_3B_FAILURES=0' "$OUT/40-host-plan.log"
grep -q 'P7_3B_FAILURES=0' "$OUT/41-sanitizers.log"
echo P7_3B_HOST_AND_SANITIZERS=PASS
