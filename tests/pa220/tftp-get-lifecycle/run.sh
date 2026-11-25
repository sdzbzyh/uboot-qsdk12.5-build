#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
OUT=/tmp/pa220-tftp-get-lifecycle
python3 - "$ROOT" "$OUT.c" <<'PY'
import importlib.util,pathlib,sys
r=pathlib.Path(sys.argv[1]);sp=importlib.util.spec_from_file_location("p6b",r/"scripts/generate-pa220-tftp-receive-patch.py");m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
p=r/"tests/pa220/tftp-get-lifecycle/host_test.c.in";s=p.read_text();assert s.count("/*__HELPERS__*/")==1
pathlib.Path(sys.argv[2]).write_text(s.replace("/*__HELPERS__*/",m.GET_STATE_HELPERS))
PY
cc -std=c99 -Wall -Wextra -Werror -O2 "$OUT.c" -o "$OUT"
"$OUT"
cc -std=c99 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "$OUT.c" -o "$OUT-san"
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 "$OUT-san"
