#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
CC=gcc
SRC="$ROOT/u-boot/board/octeon/pa220/pa220_image.c"
TEST="$ROOT/tests/pa220/image-preview/host-test.c"
"$CC" -std=c99 -Wall -Wextra -Werror -DPA220_IMAGE_HOST_TEST -I"$ROOT/u-boot/include" "$SRC" "$TEST" -o "$TMP/p7-host"
"$TMP/p7-host"
"$CC" -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -DPA220_IMAGE_HOST_TEST -I"$ROOT/u-boot/include" "$SRC" "$TEST" -o "$TMP/p7-host-san"
"$TMP/p7-host-san"
echo PA220_P7_HOST_GCC=PASS
echo PA220_P7_HOST_ASAN_UBSAN=PASS
