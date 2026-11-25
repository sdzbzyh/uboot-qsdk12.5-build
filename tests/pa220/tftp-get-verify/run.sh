#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
OUT=/tmp/pa220-tftp-get-verify
python3 - "$ROOT" "$OUT.c" <<'PY'
import importlib.util,pathlib,sys
r=pathlib.Path(sys.argv[1])
def load(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
p6b=load("p6b",r/"scripts/generate-pa220-tftp-receive-patch.py")
p6c=load("p6c",r/"scripts/generate-pa220-tftp-verify-patch.py")
p=r/"tests/pa220/tftp-get-verify/host_test.c.in";s=p.read_text()
assert s.count("/*__P6B_HELPERS__*/")==1 and s.count("/*__P6C_HELPERS__*/")==1
p6b_helpers=p6b.GET_STATE_HELPERS
old_finish = """static int tftp_get_finish(void)
{
 if (TftpGetState!=TFTP_GET_ACTIVE || TftpGetReceivedSize>TftpGetReservedSize || NetBootFileXferSize!=TftpGetReceivedSize
#ifdef CONFIG_TFTP_TSIZE
 || (TftpTsize && TftpTsize!=TftpGetReceivedSize)
#endif
 ) { TftpGetState=TFTP_GET_FAILED; TftpGetReceivedSize=0; TftpGetReservedSize=0; NetBootFileXferSize=0; (void)tftp_invalidate_get_metadata(); net_set_state(NETLOOP_FAIL); return -1; }
 TftpGetState=TFTP_GET_COMPLETE; return 0;
}
"""
assert p6b_helpers.count(old_finish)==1
p6b_helpers=p6b_helpers.replace(old_finish,p6c.GET_VERIFY_FINISH)
s=s.replace("/*__P6B_HELPERS__*/",p6b_helpers)
s=s.replace("/*__P6C_HELPERS__*/",p6c.GET_VERIFY_HELPERS)
pathlib.Path(sys.argv[2]).write_text(s)
PY
cc -std=c99 -Wall -Wextra -Werror -O2 "$OUT.c" -o "$OUT"
"$OUT"
cc -std=c99 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "$OUT.c" -o "$OUT-san"
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 "$OUT-san"
