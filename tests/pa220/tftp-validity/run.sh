#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
OUT=${TMPDIR:-/tmp}/pa220-tftp-validity-host-test
python3 - "$ROOT" "$OUT.c" <<'PY'
import importlib.util
import pathlib
import sys
root = pathlib.Path(sys.argv[1])
spec = importlib.util.spec_from_file_location(
    "pa220_tftp_validity_generator",
    root / "scripts/generate-pa220-tftp-validity-patch.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
template = (root / "tests/pa220/tftp-validity/host_test.c.in").read_text()
if template.count("/*__HELPER__*/") != 1:
    raise SystemExit("expected exactly one production helper insertion marker")
source = template.replace("/*__HELPER__*/", module.HELPER)
pathlib.Path(sys.argv[2]).write_text(source)
PY
cc -std=c99 -Wall -Wextra -Werror -O2 "$OUT.c" -o "$OUT"
"$OUT"
cc -std=c99 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer "$OUT.c" -o "$OUT-sanitized"
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 "$OUT-sanitized"
