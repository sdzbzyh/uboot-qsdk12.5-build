#!/usr/bin/env python3
import argparse, hashlib, os, pathlib, subprocess, sys, tempfile
PIN = "801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
BOARD = pathlib.Path("u-boot/board/octeon/pa220/pa220_board.c")
MAKEFILE = pathlib.Path("u-boot/board/octeon/pa220/Makefile")
def git(root, *args, env=None):
    return subprocess.run(["git", "-C", str(root), *args], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env).stdout
def replace_once(src, old, new, label):
    n = src.count(old)
    if n != 1: raise RuntimeError(label + " anchor count=" + str(n))
    return src.replace(old, new, 1)
def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--source-root",required=True); ap.add_argument("--output",required=True); a=ap.parse_args()
    root=pathlib.Path(a.source_root).resolve(); out=pathlib.Path(a.output)
    if git(root,"rev-parse","HEAD").strip()!=PIN: raise RuntimeError("wrong fixed PA-220 source pin")
    board=root/BOARD; mk=root/MAKEFILE; bs=board.read_text(); ms=mk.read_text()
    if "pa220_image_preview()" in bs and "pa220_image.o" in ms:
        if not out.is_file() or "pa220_image_preview" not in out.read_text(): raise RuntimeError("P7 patch missing after source modification")
        print("P7_ALREADY_GENERATED=verified"); print("PATCH_SHA256="+hashlib.sha256(out.read_bytes()).hexdigest()); return 0
    bs=replace_once(bs,"#include <command.h>","#include <command.h>\n#include <pa220_image.h>","image preview include")
    dispatch = '\treturn CMD_RET_USAGE;'
    dispatch_new = '''\tif (argc == 3 && !strcmp(argv[1], "image") &&
\t    !strcmp(argv[2], "preview"))
\t\treturn pa220_image_preview();

\treturn CMD_RET_USAGE;'''
    bs=replace_once(bs,dispatch,dispatch_new,"pa220 image preview dispatch")
    bs=replace_once(bs,"U_BOOT_CMD(pa220, 2, 1, do_pa220,","U_BOOT_CMD(pa220, 3, 1, do_pa220,","pa220 maxargs")
    old_usage = '"pa220 clock   - show initialized CORE, SCLK and DDR clock values");'
    new_usage = '"pa220 clock   - show initialized CORE, SCLK and DDR clock values\\n"\n\t   "pa220 image preview - parse current RAM image and print a read-only plan");'
    bs=replace_once(bs,old_usage,new_usage,"pa220 usage")
    ms=replace_once(ms,"COBJS\t= $(BOARD)_board.o","COBJS\t= $(BOARD)_board.o pa220_image.o","board object integration")
    with tempfile.TemporaryDirectory() as td:
        env=os.environ.copy(); env["GIT_INDEX_FILE"]=str(pathlib.Path(td)/"index")
        git(root,"read-tree","HEAD",env=env); git(root,"add","--",str(BOARD),str(MAKEFILE),env=env)
        board.write_text(bs); mk.write_text(ms)
        patch=git(root,"diff","--no-ext-diff","--no-renames","--binary","--",str(BOARD),str(MAKEFILE),env=env)
        if not patch or "pa220_image_preview" not in patch or "pa220_image.o" not in patch: raise RuntimeError("generated patch is incomplete")
        out.parent.mkdir(parents=True,exist_ok=True); out.write_text(patch)
    print("PA220_SOURCE_PIN="+PIN); print("P7_READ_ONLY_IMAGE_PREVIEW=integrated")
    print("P7_PATCH="+str(out)); print("P7_PATCH_SHA256="+hashlib.sha256(out.read_bytes()).hexdigest())
    print("DIFF_GENERATED_BY=git diff --binary")
    return 0
if __name__=="__main__":
    try: sys.exit(main())
    except Exception as e: print("P7 patch generation failed: "+str(e),file=sys.stderr); sys.exit(1)
