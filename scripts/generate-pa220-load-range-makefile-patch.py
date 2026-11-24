#!/usr/bin/env python3
"""Generate the OCTEON validator object Makefile diff from the pinned source."""
import argparse
import hashlib
import pathlib
import subprocess
import sys

PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
MAKEFILE = pathlib.Path("u-boot/arch/mips/cpu/octeon/Makefile")
ANCHOR = "\t\tocteon_exec.o timer.o\n"
INSERT = ANCHOR + "COBJS-y += octeon_load_range.o\n"


def run(root, *args):
    return subprocess.run(["git", "-C", str(root), *args], check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True).stdout.strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    root = pathlib.Path(args.source_root).resolve()
    if run(root, "rev-parse", "HEAD") != PIN:
        raise RuntimeError("source checkout is not the fixed PA-220 commit")
    path = root / MAKEFILE
    original = path.read_text()
    if "COBJS-y += octeon_load_range.o" not in original:
        if original.count(ANCHOR) != 1:
            raise RuntimeError("OCTEON Makefile insertion anchor is not unique")
        path.write_text(original.replace(ANCHOR, INSERT, 1))
    patch = run(root, "diff", "--no-ext-diff", "--no-renames", "--binary",
                "--", str(MAKEFILE))
    if "octeon_load_range.o" not in patch:
        raise RuntimeError("generated Makefile diff does not include validator object")
    output = pathlib.Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(patch + "\n")
    print("PIN=" + PIN)
    print("PATCH=" + str(output))
    print("PATCH_SHA256=" + hashlib.sha256(output.read_bytes()).hexdigest())
    print("OBJECT=octeon_load_range.o")
    print("DIFF_GENERATED_BY=git diff --binary")


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("load-range Makefile patch generation failed: " + str(exc),
              file=sys.stderr)
        sys.exit(1)
