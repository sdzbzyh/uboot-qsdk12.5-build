#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT HUP INT TERM
SRC="$ROOT/u-boot/board/octeon/pa220/pa220_partition.c"
TEST="$ROOT/tests/pa220/partition-inventory/host-test.c"
gcc -std=c99 -Wall -Wextra -Werror -I"$ROOT/u-boot/include" "$SRC" "$TEST" -o "$TMP/test"
"$TMP/test"
gcc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -I"$ROOT/u-boot/include" "$SRC" "$TEST" -o "$TMP/test-san"
"$TMP/test-san"
echo PA220_PARTITION_GCC=PASS
echo PA220_PARTITION_ASAN_UBSAN=PASS
