#!/bin/sh
# Build the PA-220 OCTEON U-Boot in its own source tree.
# This deliberately does not call or modify the Qualcomm build.sh path.
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PA220_SOURCE_URL="https://github.com/sdzbzyh/pa220-openwrt.git"
PA220_SOURCE_COMMIT="801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"

usage() {
    cat <<'USAGE'
Usage: ./build-pa220.sh [OCTEON_SDK_TOOLCHAIN_DIR]

Builds PA-220 U-Boot from the separately maintained OCTEON source tree.
The optional toolchain directory must contain bin/mips64-octeon-linux-gnu-gcc.
If omitted, that compiler must already be available in PATH.

Environment:
  OCTEON_TOOLCHAIN_DIR  Alternative way to specify the SDK toolchain directory.
  PA220_REPO_DIR        Source checkout directory (default: external/pa220-openwrt).
  PA220_UBOOT_DIR       Use an existing u-boot source directory instead.
  PA220_OUTPUT_DIR      Output directory (default: bin/pa220).
USAGE
}

fail() {
    echo "build-pa220: $*" >&2
    exit 1
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi
[ "$#" -le 1 ] || fail "too many arguments (use --help)"

TOOLCHAIN_DIR=${1:-${OCTEON_TOOLCHAIN_DIR:-}}
if [ -n "$TOOLCHAIN_DIR" ]; then
    case "$TOOLCHAIN_DIR" in
        /*) ;;
        *) TOOLCHAIN_DIR="$PWD/$TOOLCHAIN_DIR" ;;
    esac
    [ -d "$TOOLCHAIN_DIR/bin" ] || fail "toolchain directory must contain a bin/ directory: $TOOLCHAIN_DIR"
    TOOLCHAIN_DIR=$(CDPATH= cd -- "$TOOLCHAIN_DIR" && pwd)
    [ -x "$TOOLCHAIN_DIR/bin/mips64-octeon-linux-gnu-gcc" ] || \
        fail "mips64-octeon-linux-gnu-gcc not found under $TOOLCHAIN_DIR/bin"
fi

command -v git >/dev/null 2>&1 || fail "git is required"
command -v make >/dev/null 2>&1 || fail "make is required"
command -v nproc >/dev/null 2>&1 || fail "nproc is required by the PA-220 build script"

if [ -n "${PA220_UBOOT_DIR:-}" ]; then
    UBOOT_DIR=$PA220_UBOOT_DIR
    [ -d "$UBOOT_DIR" ] || fail "PA220_UBOOT_DIR does not exist: $UBOOT_DIR"
else
    REPO_DIR=${PA220_REPO_DIR:-$ROOT_DIR/external/pa220-openwrt}
    UBOOT_DIR="$REPO_DIR/u-boot"

    if ! git -C "$REPO_DIR" rev-parse --show-toplevel >/dev/null 2>&1; then
        [ ! -e "$REPO_DIR" ] || fail "$REPO_DIR exists but is not a Git checkout; move it aside or set PA220_UBOOT_DIR"
        mkdir -p "$(dirname -- "$REPO_DIR")"
        echo "Cloning PA-220 source at pinned revision $PA220_SOURCE_COMMIT"
        git clone --depth 1 --filter=blob:none --no-checkout "$PA220_SOURCE_URL" "$REPO_DIR"
        git -C "$REPO_DIR" sparse-checkout init --cone
        git -C "$REPO_DIR" sparse-checkout set u-boot
        git -C "$REPO_DIR" fetch --depth=1 origin "$PA220_SOURCE_COMMIT"
        git -C "$REPO_DIR" checkout --detach "$PA220_SOURCE_COMMIT"
    fi
fi

[ -f "$UBOOT_DIR/build.sh" ] || fail "PA-220 U-Boot build.sh not found: $UBOOT_DIR/build.sh"
ACTUAL_COMMIT=$(git -C "$UBOOT_DIR" rev-parse HEAD 2>/dev/null) || \
    fail "PA220_UBOOT_DIR must be inside a Git checkout"
[ "$ACTUAL_COMMIT" = "$PA220_SOURCE_COMMIT" ] || \
    fail "source revision is $ACTUAL_COMMIT; expected pinned revision $PA220_SOURCE_COMMIT (see doc/PA220.md)"

if [ -z "$TOOLCHAIN_DIR" ]; then
    command -v mips64-octeon-linux-gnu-gcc >/dev/null 2>&1 || \
        fail "mips64-octeon-linux-gnu-gcc not found; pass the OCTEON SDK toolchain directory"
fi

# Apply PA-220-specific maintenance patches maintained in this integration repo.
# Patch application is idempotent: already-applied patches are left in place.
PA220_SOURCE_ROOT=$(git -C "$UBOOT_DIR" rev-parse --show-toplevel)
PATCH_DIR="$ROOT_DIR/patches/pa220"
if [ -d "$PATCH_DIR" ]; then
    for PATCH_FILE in "$PATCH_DIR"/*.patch; do
        [ -f "$PATCH_FILE" ] || continue
        if git -C "$PA220_SOURCE_ROOT" apply --reverse --check "$PATCH_FILE" >/dev/null 2>&1; then
            echo "PA-220 patch already applied: $(basename "$PATCH_FILE")"
        else
            git -C "$PA220_SOURCE_ROOT" apply --check "$PATCH_FILE" || \
                fail "patch does not apply cleanly: $PATCH_FILE"
            git -C "$PA220_SOURCE_ROOT" apply "$PATCH_FILE" || \
                fail "failed to apply patch: $PATCH_FILE"
            echo "Applied PA-220 patch: $(basename "$PATCH_FILE")"
        fi
    done
fi

# Stage the complete P5-2C production implementation into the pinned source.
# The fixed source repository remains untouched; only this disposable checkout
# is modified by the integration build.
mkdir -p "$UBOOT_DIR/include/net"
cp "$ROOT_DIR/u-boot/common/cmd_pa220_web.c" "$UBOOT_DIR/common/cmd_pa220_web.c"
cp "$ROOT_DIR/u-boot/net/tcp.c" "$ROOT_DIR/u-boot/net/tcp_ipv4_tx.c" \
   "$ROOT_DIR/u-boot/net/http_request.c" "$ROOT_DIR/u-boot/net/http_handler.c" \
   "$ROOT_DIR/u-boot/net/http_handler_uboot.c" "$UBOOT_DIR/net/"
cp "$ROOT_DIR/u-boot/include/net/tcp.h" \
   "$ROOT_DIR/u-boot/include/net/tcp_ipv4_tx.h" \
   "$ROOT_DIR/u-boot/include/net/http_request.h" \
   "$ROOT_DIR/u-boot/include/net/http_handler.h" "$UBOOT_DIR/include/net/"
mkdir -p "$UBOOT_DIR/arch/mips/cpu/octeon" \
         "$UBOOT_DIR/arch/mips/include/asm/arch-octeon"
cp "$ROOT_DIR/u-boot/arch/mips/cpu/octeon/octeon_load_range.c" \
   "$UBOOT_DIR/arch/mips/cpu/octeon/octeon_load_range.c"
cp "$ROOT_DIR/u-boot/arch/mips/include/asm/arch-octeon/octeon_load_range.h" \
   "$UBOOT_DIR/arch/mips/include/asm/arch-octeon/octeon_load_range.h"

# Stage P7 read-only image preview into the disposable pinned-source checkout.
cp "$ROOT_DIR/u-boot/board/octeon/pa220/pa220_image.c" "$UBOOT_DIR/board/octeon/pa220/pa220_image.c"
cp "$ROOT_DIR/u-boot/include/pa220_image.h" "$UBOOT_DIR/include/pa220_image.h"
# Generate the Makefile patch with Git's diff engine (never hand-count hunks).
mkdir -p "$ROOT_DIR/reports/pa220/P5-2C"
python3 "$ROOT_DIR/scripts/generate-pa220-p5-2c-makefile-patch.py" \
    --source-root "$PA220_SOURCE_ROOT" \
    --output "$ROOT_DIR/reports/pa220/P5-2C/0006-pa220-p5-2c-makefile-integration.patch"
python3 "$ROOT_DIR/scripts/generate-pa220-load-range-makefile-patch.py" \
    --source-root "$PA220_SOURCE_ROOT" \
    --output "$ROOT_DIR/reports/pa220/P4-2/0001-pa220-load-range-makefile.patch"
mkdir -p "$ROOT_DIR/reports/pa220/P6-1A"
python3 "$ROOT_DIR/scripts/generate-pa220-tftp-validity-patch.py" \
    --source-root "$PA220_SOURCE_ROOT" \
    --output "$ROOT_DIR/reports/pa220/P6-1A/0003-pa220-tftp-get-metadata-validity.patch"

mkdir -p "$ROOT_DIR/reports/pa220/P6-1B"
python3 "$ROOT_DIR/scripts/generate-pa220-tftp-receive-patch.py" \
    --source-root "$PA220_SOURCE_ROOT" \
    --output "$ROOT_DIR/reports/pa220/P6-1B/0004-pa220-tftp-get-full-reservation.patch"

mkdir -p "$ROOT_DIR/reports/pa220/P6-1C"
python3 "$ROOT_DIR/scripts/generate-pa220-tftp-verify-patch.py" \
    --source-root "$PA220_SOURCE_ROOT" \
    --output "$ROOT_DIR/reports/pa220/P6-1C/0006-pa220-tftp-get-verification.patch"

mkdir -p "$ROOT_DIR/reports/pa220/P7-1"
python3 "$ROOT_DIR/scripts/generate-pa220-p7-image-patch.py" \
    --source-root "$PA220_SOURCE_ROOT" \
    --output "$ROOT_DIR/reports/pa220/P7-1/0001-pa220-image-preview.patch"
if [ -n "$TOOLCHAIN_DIR" ]; then
    "$UBOOT_DIR/build.sh" "$TOOLCHAIN_DIR"
else
    "$UBOOT_DIR/build.sh"
fi

BUILT_IMAGE="$UBOOT_DIR/u-boot-octeon_pa220.bin"
[ -s "$BUILT_IMAGE" ] || fail "build did not produce $BUILT_IMAGE"

OUTPUT_DIR=${PA220_OUTPUT_DIR:-$ROOT_DIR/bin/pa220}
mkdir -p "$OUTPUT_DIR"
cp -f "$BUILT_IMAGE" "$OUTPUT_DIR/u-boot-octeon_pa220.bin"
echo "PA-220 U-Boot image: $OUTPUT_DIR/u-boot-octeon_pa220.bin"
