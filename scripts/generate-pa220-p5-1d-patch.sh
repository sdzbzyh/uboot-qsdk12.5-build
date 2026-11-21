#!/bin/bash
set -euo pipefail

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BASE_DIR="$RUNNER_TEMP/pa220-p5-1d-patch-base"
SOURCE_COMMIT="801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
PATCH_OUT="$ROOT_DIR/patches/pa220/0005-pa220-native-tcp-ipv4-ethernet-output.patch"

if [ -e "$BASE_DIR" ]; then
	echo "Refusing to reuse existing temporary source tree: $BASE_DIR" >&2
	exit 1
fi

git clone --no-checkout --filter=blob:none https://github.com/IronDemiGod/pa220-openwrt.git "$BASE_DIR"
git -C "$BASE_DIR" fetch --depth=1 origin "$SOURCE_COMMIT"
git -C "$BASE_DIR" checkout --detach "$SOURCE_COMMIT"
ACTUAL=$(git -C "$BASE_DIR" rev-parse HEAD)
[ "$ACTUAL" = "$SOURCE_COMMIT" ]

# Reconstruct the exact source state after the already accepted P5-1C patch series.
for PATCH_FILE in "$ROOT_DIR"/patches/pa220/000[1-4]-*.patch; do
	[ -f "$PATCH_FILE" ] || continue
	git -C "$BASE_DIR" apply --check "$PATCH_FILE"
	git -C "$BASE_DIR" apply "$PATCH_FILE"
done

git -C "$BASE_DIR" config user.name "PA-220 patch generator"
git -C "$BASE_DIR" config user.email "pa220-patch-generator@users.noreply.github.com"
git -C "$BASE_DIR" add -A
git -C "$BASE_DIR" commit -m "temporary baseline: accepted PA-220 P5-1C patch series"

UBOOT="$BASE_DIR/u-boot"
cp "$ROOT_DIR/u-boot/common/cmd_pa220_web.c" "$UBOOT/common/cmd_pa220_web.c"
cp "$ROOT_DIR/u-boot/net/tcp_ipv4_tx.c" "$UBOOT/net/tcp_ipv4_tx.c"
cp "$ROOT_DIR/u-boot/include/net/tcp_ipv4_tx.h" "$UBOOT/include/net/tcp_ipv4_tx.h"

python3 - "$UBOOT/include/net.h" "$UBOOT/net/net.c" "$UBOOT/net/Makefile" "$UBOOT/net/tcp.c" <<'PY'
import re
import sys

net_h, net_c, makefile, tcp_c = sys.argv[1:]

def replace_once(path, pattern, replacement):
    with open(path, "r", encoding="utf-8") as f:
        data = f.read()
    updated, count = re.subn(pattern, replacement, data, count=1)
    if count != 1:
        raise SystemExit("expected exactly one source match in %s, found %d" %
                         (path, count))
    with open(path, "w", encoding="utf-8") as f:
        f.write(updated)

def replace_literal_once(path, old, new):
    with open(path, "r", encoding="utf-8") as f:
        data = f.read()
    count = data.count(old)
    if count != 1:
        raise SystemExit("expected exactly one literal source match in %s, found %d" %
                         (path, count))
    with open(path, "w", encoding="utf-8") as f:
        f.write(data.replace(old, new, 1))

replace_once(
    net_h,
    r"typedef void tcp_rxhand_f\(const uchar \*segment, unsigned len,\s*"
    r"IPaddr_t src_ip, IPaddr_t dst_ip\);",
    "typedef void tcp_rxhand_f(const uchar *segment, unsigned len,\n"
    "\t\t  IPaddr_t src_ip, IPaddr_t dst_ip,\n"
    "\t\t  const uchar src_mac[6]);"
)
replace_once(
    net_c,
    r"tcp_packet_handler\(\(uchar \*\)ip \+ IP_HDR_SIZE,\s*"
    r"tcp_segment_len, src_ip, dst_ip\);",
    "tcp_packet_handler((uchar *)ip + IP_HDR_SIZE,\n"
    "\t\t\t\t   tcp_segment_len, src_ip, dst_ip, et->et_src);"
)

old_syn = (
    "\t\tif ((v.flags & TCP_FLAG_SYN) && v.sequence == c->irs) {\n"
    "\t\t\tif (c->pending)\n"
    "\t\t\t\ttcp_emit(c, c->retry_flags, c->retry_seq,\n"
    "\t\t\t\t\t c->rcv_nxt, NULL, 0, 1, emit, arg);\n"
    "\t\t\treturn 1;\n"
    "\t\t}"
)
new_syn = (
    "\t\tif ((v.flags & TCP_FLAG_SYN) && v.sequence == c->irs) {\n"
    "\t\t\tif (c->pending &&\n"
    "\t\t\t    tcp_emit(c, c->retry_flags, c->retry_seq,\n"
    "\t\t\t\t     c->rcv_nxt, NULL, 0, 1, emit, arg))\n"
    "\t\t\t\treturn -1;\n"
    "\t\t\treturn 1;\n"
    "\t\t}"
)
replace_literal_once(tcp_c, old_syn, new_syn)

old_poll = (
    "\tif (tcp_emit(c, c->retry_flags, c->retry_seq, c->rcv_nxt,\n"
    "\t\t     c->retry_len ? c->retry_data : NULL, c->retry_len,\n"
    "\t\t     !!(c->retry_flags & TCP_FLAG_SYN), emit, arg))\n"
    "\t\treturn -1;\n"
    "\tc->retries++;\n"
    "\tc->retry_deadline = now + TCP_CORE_RETRANSMIT_MS;\n"
    "\treturn 1;"
)
new_poll = (
    "\tif (tcp_emit(c, c->retry_flags, c->retry_seq, c->rcv_nxt,\n"
    "\t\t     c->retry_len ? c->retry_data : NULL, c->retry_len,\n"
    "\t\t     !!(c->retry_flags & TCP_FLAG_SYN), emit, arg)) {\n"
    "\t\tc->retries++;\n"
    "\t\tif (c->retries >= TCP_CORE_MAX_RETRIES) {\n"
    "\t\t\ttcp_abort_connection(c);\n"
    "\t\t\treturn 1;\n"
    "\t\t}\n"
    "\t\tc->retry_deadline = now + TCP_CORE_RETRANSMIT_MS;\n"
    "\t\treturn -1;\n"
    "\t}\n"
    "\tc->retries++;\n"
    "\tc->retry_deadline = now + TCP_CORE_RETRANSMIT_MS;\n"
    "\treturn 1;"
)
replace_literal_once(tcp_c, old_poll, new_poll)

with open(makefile, "r", encoding="utf-8") as f:
    lines = f.readlines()
if not any("tcp_ipv4_tx.o" in line for line in lines):
    matches = [i for i, line in enumerate(lines)
               if re.search(r"COBJS-\$\(CONFIG_CMD_NET\).*\+=\s*tcp\.o", line)]
    if len(matches) != 1:
        raise SystemExit("expected one CONFIG_CMD_NET tcp.o Makefile entry")
    i = matches[0]
    m = re.match(r"^(\s*COBJS-\$\(CONFIG_CMD_NET\)\s*\+=\s*)tcp\.o", lines[i])
    if not m:
        raise SystemExit("could not preserve tcp.o Makefile assignment")
    lines.insert(i + 1, m.group(1) + "tcp_ipv4_tx.o\n")
    with open(makefile, "w", encoding="utf-8") as f:
        f.writelines(lines)
PY

git -C "$BASE_DIR" add -A
git -C "$BASE_DIR" diff --cached --check
git -C "$BASE_DIR" diff --cached --binary --full-index HEAD > "$PATCH_OUT"
test -s "$PATCH_OUT"
git -C "$BASE_DIR" apply --reverse --check "$PATCH_OUT"
sha256sum "$PATCH_OUT"
git -C "$BASE_DIR" diff --cached --stat
