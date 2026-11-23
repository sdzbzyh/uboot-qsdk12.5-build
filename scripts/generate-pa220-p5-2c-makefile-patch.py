#!/usr/bin/env python3
"""Generate the PA-220 P5-2C Makefile integration patch from the pinned source.

The unified diff is emitted by Git itself; hunk counts are never authored here.
"""
import argparse
import pathlib
import subprocess
import sys

PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
COMMON = pathlib.Path("u-boot/common/Makefile")
NET = pathlib.Path("u-boot/net/Makefile")


def run(root, *args):
    return subprocess.run(
        ["git", "-C", str(root), *args],
        check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
    ).stdout.strip()


def replace_once(path, old, new):
    data = path.read_text()
    if new.strip() in data:
        return
    if data.count(old) != 1:
        raise RuntimeError(f"{path}: expected exactly one insertion anchor {old!r}")
    path.write_text(data.replace(old, new, 1))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-root", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    root = pathlib.Path(args.source_root).resolve()
    if run(root, "rev-parse", "HEAD") != PIN:
        raise RuntimeError("source checkout is not the fixed PA-220 commit")
    common, net = root / COMMON, root / NET
    if not common.is_file() or not net.is_file():
        raise RuntimeError("pinned U-Boot Makefiles are missing")
    replace_once(
        common, "# core command\n",
        "# core command\nCOBJS-y += cmd_pa220_web.o\n"
    )
    replace_once(
        net, "COBJS\t:= $(sort $(COBJS-y))\n",
        "COBJS-$(CONFIG_CMD_NET) += tcp.o tcp_ipv4_tx.o http_request.o http_handler.o http_handler_uboot.o\n\n"
        "COBJS\t:= $(sort $(COBJS-y))\n"
    )
    patch = run(root, "diff", "--no-ext-diff", "--no-renames", "--binary",
                "--", str(COMMON), str(NET))
    if not patch:
        raise RuntimeError("Makefile integration diff is unexpectedly empty")
    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(patch + "\n")
    for required in ("cmd_pa220_web.o", "tcp.o", "tcp_ipv4_tx.o",
                     "http_request.o", "http_handler.o", "http_handler_uboot.o"):
        if required not in patch:
            raise RuntimeError(f"generated patch omitted {required}")
    print(f"PIN={PIN}")
    print(f"PATCH={output}")
    print(f"PATCH_SHA256={__import__('hashlib').sha256(output.read_bytes()).hexdigest()}")
    print("MAKEFILE_OBJECTS=6")
    print("DIFF_GENERATED_BY=git diff --binary")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print(f"makefile patch generation failed: {exc}", file=sys.stderr)
        sys.exit(1)
