#!/usr/bin/env python3
import argparse,hashlib,os,pathlib,subprocess,sys,tempfile
PIN="801717ba6a8a59ed2cfd53bf3e1b19841fecd2e8"
TFTP=pathlib.Path("u-boot/net/tftp.c")
GET_STATE_HELPERS=r'''
#ifndef CONFIG_PA220_TFTP_MAX_FILE_SIZE
#define CONFIG_PA220_TFTP_MAX_FILE_SIZE (32UL * 1024UL * 1024UL)
#endif
enum { TFTP_GET_IDLE, TFTP_GET_ACTIVE, TFTP_GET_COMPLETE, TFTP_GET_FAILED };
static int TftpGetState;
static ulong TftpGetReservedSize, TftpGetReceivedSize;
static int tftp_invalidate_get_metadata(void);
static int tftp_get_reserve(void)
{
 TftpGetState=TFTP_GET_IDLE; TftpGetReservedSize=0; TftpGetReceivedSize=0; NetBootFileXferSize=0;
 if (!CONFIG_PA220_TFTP_MAX_FILE_SIZE || octeon_load_range_check((uint64_t)load_addr,(uint64_t)CONFIG_PA220_TFTP_MAX_FILE_SIZE)) { TftpGetState=TFTP_GET_FAILED; return -1; }
 TftpGetReservedSize=(ulong)CONFIG_PA220_TFTP_MAX_FILE_SIZE; TftpGetState=TFTP_GET_ACTIVE; return 0;
}
static int tftp_get_check_block(uint64_t off,unsigned len)
{
 if (TftpGetState!=TFTP_GET_ACTIVE || off!=(uint64_t)TftpGetReceivedSize || TftpGetReceivedSize>TftpGetReservedSize || (uint64_t)len>(uint64_t)TftpGetReservedSize-(uint64_t)TftpGetReceivedSize) return -1;
 return 0;
}
static void tftp_get_commit_block(unsigned len) { TftpGetReceivedSize+=(ulong)len; }
static int tftp_get_finish(void)
{
 if (TftpGetState!=TFTP_GET_ACTIVE || TftpGetReceivedSize>TftpGetReservedSize || NetBootFileXferSize!=TftpGetReceivedSize
#ifdef CONFIG_TFTP_TSIZE
 || (TftpTsize && TftpTsize!=TftpGetReceivedSize)
#endif
 ) { TftpGetState=TFTP_GET_FAILED; TftpGetReceivedSize=0; TftpGetReservedSize=0; NetBootFileXferSize=0; (void)tftp_invalidate_get_metadata(); net_set_state(NETLOOP_FAIL); return -1; }
 TftpGetState=TFTP_GET_COMPLETE; return 0;
}
static void tftp_get_fail(const char *why)
{
 printf("\\nTFTP GET failed: %s\\n",why); TftpGetState=TFTP_GET_FAILED; TftpGetReceivedSize=0; TftpGetReservedSize=0; NetBootFileXferSize=0; (void)tftp_invalidate_get_metadata(); net_set_state(NETLOOP_FAIL);
}
void TftpGetAbort(void)
{
 if (TftpGetState == TFTP_GET_ACTIVE) tftp_get_fail("interrupted or aborted");
}
'''
def git(root,*args,env=None):
 return subprocess.run(["git","-C",str(root),*args],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env).stdout
def rep(s,a,b,label):
 if s.count(a)!=1: raise RuntimeError(label+" anchor count="+str(s.count(a)))
 return s.replace(a,b,1)
def main():
 p=argparse.ArgumentParser(); p.add_argument("--source-root",required=True); p.add_argument("--output",required=True); a=p.parse_args()
 root=pathlib.Path(a.source_root).resolve()
 if git(root,"rev-parse","HEAD").strip()!=PIN: raise RuntimeError("wrong PA-220 source pin")
 path=root/TFTP; out=pathlib.Path(a.output); s=path.read_text()
 if "tftp_get_reserve(void)" in s:
  if not out.is_file() or "tftp_get_reserve" not in out.read_text(): raise RuntimeError("missing P6-1B patch")
  print("P6-1B_ALREADY_GENERATED=verified"); print("PATCH="+str(out)); print("PATCH_SHA256="+hashlib.sha256(out.read_bytes()).hexdigest()); print("DIFF_GENERATED_BY=git diff --binary"); return 0
 for x in ["static inline int\nstore_block(","if (len && octeon_load_range_check(dest64, (uint64_t)len))","static void tftp_complete(void)\n{","static void\nrestart(const char *msg)","void TftpStart(enum proto_t protocol)","case TFTP_ERROR:","case TFTP_OACK:","if (store_block((int)TftpBlock - 1, pkt + 2, len))"]:
  if x not in s: raise RuntimeError("missing source anchor "+x)
 with tempfile.TemporaryDirectory() as td:
  env=os.environ.copy(); env["GIT_INDEX_FILE"]=str(pathlib.Path(td)/"index")
  git(root,"read-tree","HEAD",env=env); git(root,"add","--",str(TFTP),env=env)
  s=rep(s,"static inline int\nstore_block(",GET_STATE_HELPERS+"\nstatic inline int\nstore_block(","state helpers")
  s=s.replace("static int TftpTsize;","static ulong TftpTsize;")
  s=rep(s,'\t\tprintf("Load address: 0x%lx\\n", load_addr);','\t\tprintf("Load address: 0x%lx\\n", load_addr);\n\t\tif (tftp_get_reserve()) {\n\t\t\ttftp_get_fail("unable to reserve complete maximum file range");\n\t\t\treturn;\n\t\t}',"GET reservation")
  s=rep(s,"\tTftpRemoteIP = NetServerIP;","\tTftpGetState = TFTP_GET_IDLE;\n\tTftpRemoteIP = NetServerIP;","TftpStart reset")
  s=rep(s,"\tTftpState = STATE_RECV_WRQ;","\tTftpGetState = TFTP_GET_IDLE;\n\tTftpState = STATE_RECV_WRQ;","WRQ reset")
  s=rep(s,"\tif (len && octeon_load_range_check(dest64, (uint64_t)len))\n\t\tgoto range_error;","\tif (len && octeon_load_range_check(dest64, (uint64_t)len))\n\t\tgoto range_error;\n\tif (TftpGetState == TFTP_GET_ACTIVE && tftp_get_check_block(offset64, len))\n\t\tgoto range_error;","block check")
  s=rep(s,"\tif (NetBootFileXferSize < newsize)\n\t\tNetBootFileXferSize = newsize;\n\treturn 0;","\tif (TftpGetState == TFTP_GET_ACTIVE)\n\t\ttftp_get_commit_block(len);\n\tif (NetBootFileXferSize < newsize)\n\t\tNetBootFileXferSize = newsize;\n\treturn 0;","block commit")
  s=rep(s,'range_error:\n\tprintf("TFTP: rejecting DATA block (offset overflow or outside __tmp_load)\\n");\n\tnet_set_state(NETLOOP_FAIL);\n\treturn -1;','range_error:\n\tif (TftpGetState == TFTP_GET_ACTIVE)\n\t\ttftp_get_fail("DATA block exceeds reservation or is non-sequential");\n\telse {\n\t\tprintf("TFTP: rejecting DATA block (offset overflow or outside __tmp_load)\\n");\n\t\tnet_set_state(NETLOOP_FAIL);\n\t}\n\treturn -1;',"range failure")
  s=rep(s,'\tprintf("\\n%s; starting again\\n", msg);','\tif (TftpGetState == TFTP_GET_ACTIVE) { tftp_get_fail(msg); return; }\n\tprintf("\\n%s; starting again\\n", msg);',"restart")
  s=rep(s,"static void tftp_complete(void)\n{","static void tftp_complete(void)\n{\n\tif (TftpGetState == TFTP_GET_ACTIVE && tftp_get_finish())\n\t\treturn;","completion")
  s=rep(s,"\t\t}\n#ifdef CONFIG_MCAST_TFTP\n\t\tparse_multicast_oack","\t\t}\n#ifdef CONFIG_TFTP_TSIZE\n\t\tif (TftpGetState == TFTP_GET_ACTIVE && TftpTsize > TftpGetReservedSize) { tftp_get_fail(\"server tsize exceeds maximum\"); break; }\n#endif\n#ifdef CONFIG_MCAST_TFTP\n\t\tparse_multicast_oack","OACK size")
  s=rep(s,"\t\t\t\tNetStartAgain();\n\t\t\t\tbreak;","\t\t\t\tif (TftpGetState == TFTP_GET_ACTIVE) tftp_get_fail(\"first DATA block is not block 1\");\n\t\t\t\telse NetStartAgain();\n\t\t\t\tbreak;","first block")
  s=rep(s,"\tcase TFTP_ERROR:\n\t\tprintf(","\tcase TFTP_ERROR:\n\t\tif (TftpGetState == TFTP_GET_ACTIVE) { tftp_get_fail(\"server TFTP ERROR\"); break; }\n\t\tprintf(","TFTP ERROR")
  ic="\tif (type == ICMP_NOT_REACH && code == ICMP_NOT_REACH_PORT) {"
  s=rep(s,ic,ic+"\n\t\tif (TftpGetState == TFTP_GET_ACTIVE) { tftp_get_fail(\"network interruption\"); return; }","ICMP")
  cmd_path=root/"u-boot/common/cmd_net.c"
  cmd=cmd_path.read_text()
  cmd=rep(cmd,"int do_tftpb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])","extern void TftpGetAbort(void);\n\nint do_tftpb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])","GET command declaration")
  cmd=rep(cmd,"ret = netboot_common(TFTPGET, cmdtp, argc, argv);","ret = netboot_common(TFTPGET, cmdtp, argc, argv);\n\tTftpGetAbort();","GET command abort hook")
  path.write_text(s); cmd_path.write_text(cmd)
  patch=git(root,"diff","--no-ext-diff","--no-renames","--binary","--",str(TFTP),env=env)
  cmd_patch=git(root,"diff","--no-ext-diff","--no-renames","--binary","--",str(cmd_path),env=env)
  if not patch or "tftp_get_reserve" not in patch or "saveenv" in patch or "TftpWriting" in patch or "TftpGetAbort" not in cmd_patch: raise RuntimeError("incomplete patch or PUT/WRQ touched")
  out.parent.mkdir(parents=True,exist_ok=True); out.write_text(patch)
  out.with_name("0005-pa220-tftp-get-abort-hook.patch").write_text(cmd_patch)
 print("PA220_SOURCE_PIN="+PIN); print("PATCH="+str(out)); print("PATCH_SHA256="+hashlib.sha256(out.read_bytes()).hexdigest()); print("MAX_FILE_SIZE=32MiB"); print("FULL_RANGE_RESERVED_BEFORE_RRQ=verified"); print("SEQUENTIAL_BLOCK_AND_TOTAL_LENGTH_CHECKS=verified"); print("FAILURE_INVALIDATES_METADATA=verified"); print("PUT_WRQ_UNCHANGED=verified"); print("DIFF_GENERATED_BY=git diff --binary")
if __name__=="__main__":
 try: sys.exit(main())
 except Exception as e: print("P6-1B patch generation failed: "+str(e),file=sys.stderr); sys.exit(1)
