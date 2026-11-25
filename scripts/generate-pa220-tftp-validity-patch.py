#!/usr/bin/env python3
"""Generate the P6-1A TFTP GET metadata-lifecycle patch from pinned PA-220 source."""
import argparse
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile

PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
TFTP = pathlib.Path("u-boot/net/tftp.c")

HELPER = r'''static int
tftp_invalidate_get_metadata(void)
{
	int failed = 0;

	/*
	 * setenv(name, NULL) returns 1 when the variable does not exist.
	 * Probe first, and attempt both deletions even if the first fails.
	 */
	if (getenv("filesize") && setenv("filesize", NULL) != 0)
		failed = 1;
	if (getenv("fileaddr") && setenv("fileaddr", NULL) != 0)
		failed = 1;

	/* Fail closed unless both volatile result markers are now absent. */
	if (getenv("filesize") || getenv("fileaddr"))
		failed = 1;

	return failed ? -1 : 0;
}'''
GET_GATE = r'''		if (tftp_invalidate_get_metadata()) {
			puts("\nTFTP GET: unable to invalidate previous file metadata; refusing request\n");
			net_set_state(NETLOOP_FAIL);
			return;
		}'''


def git(root, *args, env=None):
    return subprocess.run(
        ["git", "-C", str(root), *args], check=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env
    ).stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source-root", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()
    root = pathlib.Path(args.source_root).resolve()
    if git(root, "rev-parse", "HEAD").strip() != PIN:
        raise RuntimeError("source checkout is not the fixed PA-220 commit")

    path = root / TFTP
    source = path.read_text()
    if "tftp_invalidate_get_metadata" in source:
        out = pathlib.Path(args.output)
        if not out.is_file():
            raise RuntimeError("P6-1A source is already modified but its generated patch is missing")
        existing = out.read_text()
        if "tftp_invalidate_get_metadata" not in existing or "unable to invalidate previous file metadata" not in existing:
            raise RuntimeError("existing P6-1A patch does not match the applied source")
        print("P6-1A_ALREADY_GENERATED=verified")
        print("PATCH=" + str(out))
        print("PATCH_SHA256=" + hashlib.sha256(out.read_bytes()).hexdigest())
        print("DIFF_GENERATED_BY=git diff --binary")
        return 0
    if source.count("void TftpStart(enum proto_t protocol)") != 1:
        raise RuntimeError("TftpStart anchor is missing or ambiguous")
    if source.count('\t\tprintf("Load address: 0x%lx\\n", load_addr);') != 1:
        raise RuntimeError("TFTP GET branch anchor is missing or ambiguous")
    if "TftpWriting = (protocol == TFTPPUT);" not in source:
        raise RuntimeError("TFTP PUT/GET split not found")

    # Keep the P4-2 working-tree changes as the baseline without changing the
    # source checkout's real index. The resulting patch contains only P6-1A.
    with tempfile.TemporaryDirectory(prefix="pa220-p6-index-") as td:
        index = pathlib.Path(td) / "index"
        env = os.environ.copy()
        env["GIT_INDEX_FILE"] = str(index)
        git(root, "read-tree", "HEAD", env=env)
        git(root, "add", "--", str(TFTP), env=env)

        start = source.index("void TftpStart(enum proto_t protocol)")
        source = source[:start] + HELPER + "\n\n" + source[start:]
        anchor = '\t\tprintf("Load address: 0x%lx\\n", load_addr);'
        source = source.replace(anchor, GET_GATE + "\n" + anchor, 1)
        path.write_text(source)

        patch = git(root, "diff", "--no-ext-diff", "--no-renames",
                    "--binary", "--", str(TFTP), env=env)
        if not patch or "tftp_invalidate_get_metadata" not in patch:
            raise RuntimeError("P6-1A diff is empty or missing the production helper")
        if patch.count("tftp_invalidate_get_metadata") < 2:
            raise RuntimeError("P6-1A helper and GET gate are not both present")
        if "saveenv" in patch:
            raise RuntimeError("P6-1A must not call saveenv")

    out = pathlib.Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(patch)
    print("PA220_SOURCE_PIN=" + PIN)
    print("PATCH=" + str(out))
    print("PATCH_SHA256=" + hashlib.sha256(out.read_bytes()).hexdigest())
    print("GET_ONLY_GATE=verified")
    print("INDEPENDENT_DELETE_ATTEMPTS=filesize,fileaddr")
    print("FINAL_ABSENCE_CHECK=verified")
    print("DIFF_GENERATED_BY=git diff --binary")


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print("P6-1A patch generation failed: " + str(exc), file=sys.stderr)
        sys.exit(1)
