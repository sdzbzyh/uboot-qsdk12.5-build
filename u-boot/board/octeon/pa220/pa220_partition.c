/* SPDX-License-Identifier: GPL-2.0+ */
/* Bounded, read-only DOS/EBR and GPT inventory. No block write interface. */
#ifdef PA220_PARTITION_HOST_TEST
#include <stdint.h>
#include <string.h>
#else
#include <common.h>
#endif
#include <pa220_partition.h>
#define MBR_SIG 0xaa55
#define EXT_MAX 16
#define GPT_MAX_ENTRIES 128
#define GPT_MAX_ENTRY_SIZE 256
#define GPT_HEADER_MIN 92
static uint16_t rd16(const uint8_t *p){ return (uint16_t)p[0] | (uint16_t)p[1]<<8; }
static uint32_t rd32(const uint8_t *p){ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static uint64_t rd64(const uint8_t *p){ return (uint64_t)rd32(p) | (uint64_t)rd32(p+4)<<32; }
static int add64(uint64_t a,uint64_t b,uint64_t *r){ if (UINT64_MAX-a < b) return 0; *r=a+b; return 1; }
static uint32_t crc32_bytes(const uint8_t *p,uint32_t n){
 uint32_t c=0xffffffffU,i; unsigned int b;
 for(i=0;i<n;i++){ c^=p[i]; for(b=0;b<8;b++) c=(c>>1)^((c&1)?0xedb88320U:0); }
 return c^0xffffffffU;
}
static int read1(pa220_sector_read_fn fn,void *ctx,uint64_t lba,uint64_t cap,uint8_t *b){
 if(lba>=cap) return -1;
 return fn(ctx,lba,b)?-1:0;
}
static int append(struct pa220_partition_table *o,uint64_t s,uint64_t n,uint8_t type,uint8_t kind){
 uint64_t e,pe; uint32_t i;
 if(!n || !add64(s,n,&e) || e>o->capacity || o->count>=PA220_PART_MAX) return 0;
 for(i=0;i<o->count;i++){ if(!add64(o->parts[i].start_lba,o->parts[i].sectors,&pe)) return 0;
  if(s<pe && o->parts[i].start_lba<e) return 0;
 }
 o->parts[o->count].index=o->count+1; o->parts[o->count].start_lba=s; o->parts[o->count].sectors=n;
 o->parts[o->count].type=type; o->parts[o->count].kind=kind; o->count++; return 1;
}
static int empty_entry(const uint8_t *e){ unsigned int i; for(i=0;i<16;i++) if(e[i]) return 0; return 1; }
static int valid_boot(const uint8_t *e){ return e[0]==0 || e[0]==0x80; }
static int scan_mbr(pa220_sector_read_fn fn,void *ctx,struct pa220_partition_table *o,const uint8_t *m){
 uint64_t ext=0,extn=0,cur,base,end,next,ls,ln,le,linkn; uint8_t sec[512]; uint32_t i,hops;
 for(i=0;i<4;i++){ const uint8_t *e=m+446+i*16; uint8_t t=e[4]; uint64_t s=rd32(e+8),n=rd32(e+12),x;
  if(empty_entry(e)) continue;
  if(!valid_boot(e) || !t || !s || !n || !add64(s,n,&x) || x>o->capacity) return PA220_PT_CORRUPT;
  if(t==0xee) return PA220_PT_AMBIGUOUS;
  if(t==0x05 || t==0x0f || t==0x85){ if(ext) return PA220_PT_AMBIGUOUS; ext=s; extn=n; if(!add64(ext,extn,&end)||end>o->capacity) return PA220_PT_CORRUPT; }
  else if(!append(o,s,n,t,PA220_PART_PRIMARY)) return PA220_PT_CORRUPT;
 }
 if(!ext) return PA220_PT_OK;
 for(i=0;i<o->count;i++){ uint64_t pe; if(!add64(o->parts[i].start_lba,o->parts[i].sectors,&pe) || (o->parts[i].start_lba<end && ext<pe)) return PA220_PT_CORRUPT; }
 cur=ext;
 {
  uint64_t visited[EXT_MAX]; uint32_t used=0;
  for(hops=0;hops<EXT_MAX;hops++){
   const uint8_t *a,*l;
   if(cur<ext || cur>=end || cur>=o->capacity) return PA220_PT_CORRUPT;
   for(i=0;i<o->count;i++){ uint64_t pe; if(!add64(o->parts[i].start_lba,o->parts[i].sectors,&pe) || (cur>=o->parts[i].start_lba && cur<pe)) return PA220_PT_CORRUPT; }
   for(i=0;i<used;i++) if(visited[i]==cur) return PA220_PT_CORRUPT;
   visited[used++]=cur;
   if(read1(fn,ctx,cur,o->capacity,sec)<0) return PA220_PT_IO;
   if(rd16(sec+510)!=MBR_SIG) return PA220_PT_CORRUPT;
   a=sec+446; l=a+16;
   if(!empty_entry(a+32) || !empty_entry(a+48)) return PA220_PT_CORRUPT;
   if(!empty_entry(a)){
    uint8_t t=a[4]; ls=rd32(a+8); ln=rd32(a+12);
    if(!valid_boot(a)||!t||!ls||!ln||t==0x05||t==0x0f||t==0x85) return PA220_PT_CORRUPT;
    if(!add64(cur,ls,&base)||!add64(base,ln,&le)||base<ext||le>end) return PA220_PT_CORRUPT;
    if(!append(o,base,ln,t,PA220_PART_LOGICAL)) return PA220_PT_CORRUPT;
   }
   if(empty_entry(l)) return PA220_PT_OK;
   if(!valid_boot(l)|| (l[4]!=0x05 && l[4]!=0x0f && l[4]!=0x85)) return PA220_PT_CORRUPT;
   linkn=rd32(l+12); next=rd32(l+8);
   if(!linkn || !add64(ext,next,&cur) || !add64(cur,linkn,&le) || cur<ext || le>end) return PA220_PT_CORRUPT;
  }
 }
 return PA220_PT_CORRUPT;
}
static int pa220_gpt_read_header(pa220_sector_read_fn fn,void *ctx,uint64_t cap,uint64_t lba,
                      uint8_t *h,uint64_t *entries_lba,uint32_t *count,uint32_t *esz,uint32_t *array_crc){
 uint32_t hs,stored,calc,bytes; uint8_t tmp[512];
 if(read1(fn,ctx,lba,cap,h)<0 || memcmp(h,"EFI PART",8)) return 0;
 hs=rd32(h+12); if(hs<GPT_HEADER_MIN||hs>512||rd32(h+8)!=0x00010000U||rd32(h+20)!=0) return 0;
 memcpy(tmp,h,512); stored=rd32(tmp+16); memset(tmp+16,0,4); calc=crc32_bytes(tmp,hs); if(calc!=stored) return 0;
 if(rd64(h+24)!=lba || rd64(h+32)>=cap || rd64(h+32)==lba) return 0;
 if(rd64(h+40)>rd64(h+48) || rd64(h+48)>=cap) return 0;
 *entries_lba=rd64(h+72); *count=rd32(h+80); *esz=rd32(h+84); *array_crc=rd32(h+88);
 if(!*count||*count>GPT_MAX_ENTRIES||*esz<128||*esz>GPT_MAX_ENTRY_SIZE||(*esz&7)) return 0;
 if(*count > UINT32_MAX / *esz) return 0;
 bytes=(*count)*(*esz);
 if(*entries_lba>=cap || (uint64_t)((bytes+511U)/512U)>cap-*entries_lba) return 0;
 return 1;
}
static int scan_gpt(pa220_sector_read_fn fn,void *ctx,struct pa220_partition_table *o){
 static uint8_t pa[GPT_MAX_ENTRIES*GPT_MAX_ENTRY_SIZE],ba[GPT_MAX_ENTRIES*GPT_MAX_ENTRY_SIZE];
 uint8_t p[512],ph[512],bh[512],sec[512];
 uint64_t pl,bl,first,last,array_sectors,array_end; uint32_t pc,bc,pe,be,pcrc,bcrc,bytes,i,j;
 if(read1(fn,ctx,0,o->capacity,p)<0) return PA220_PT_IO;
 if(rd16(p+510)!=MBR_SIG) return PA220_PT_UNKNOWN;
 if(p[446]!=0 || p[450]!=0xee || rd32(p+454)!=1 ||
    rd32(p+458)!=(o->capacity-1>0xffffffffULL?0xffffffffU:(uint32_t)(o->capacity-1))) return PA220_PT_AMBIGUOUS;
 for(i=1;i<4;i++) if(!empty_entry(p+446+i*16)) return PA220_PT_AMBIGUOUS;
 bl=o->capacity-1;
 if(!pa220_gpt_read_header(fn,ctx,o->capacity,1,ph,&pl,&pc,&pe,&pcrc) ||
    !pa220_gpt_read_header(fn,ctx,o->capacity,bl,bh,&bl,&bc,&be,&bcrc)) return PA220_PT_CORRUPT;
 if(rd64(ph+32)!=o->capacity-1 || rd64(bh+32)!=1 ||
    memcmp(ph+56,bh+56,16)||rd32(ph+8)!=rd32(bh+8)||rd32(ph+12)!=rd32(bh+12)||rd64(ph+40)!=rd64(bh+40)||rd64(ph+48)!=rd64(bh+48)||
    pc!=bc||pe!=be||pcrc!=bcrc||pl==bl ||
    !memcmp(ph+56,(uint8_t[16]){0},16)) return PA220_PT_CORRUPT;
 bytes=pc*pe; array_sectors=(bytes+511U)/512U;
 first=rd64(ph+40); last=rd64(ph+48);
 memcpy(o->disk_guid,ph+56,16);
 if(!add64(pl,array_sectors,&array_end)||pl!=2||array_end>first ||
    bl<=last || !add64(bl,array_sectors,&array_end)||array_end!=o->capacity-1) return PA220_PT_CORRUPT;
 memset(pa,0,sizeof(pa)); memset(ba,0,sizeof(ba));
 for(i=0;i<(bytes+511U)/512U;i++){
  if(read1(fn,ctx,pl+i,o->capacity,sec)<0) return PA220_PT_IO;
  memcpy(pa+i*512,sec,512);
  if(read1(fn,ctx,bl+i,o->capacity,sec)<0) return PA220_PT_IO;
  memcpy(ba+i*512,sec,512);
 }
 if(crc32_bytes(pa,bytes)!=pcrc||crc32_bytes(ba,bytes)!=bcrc||memcmp(pa,ba,bytes)) return PA220_PT_CORRUPT;
 for(i=0;i<pc;i++){
  const uint8_t *e=pa+i*pe; uint64_t s,en,n;
  if(!memcmp(e,(uint8_t[16]){0},16)) continue;
  s=rd64(e+32); en=rd64(e+40);
  if(s<first||en>last||en<s||!add64(en-s,1,&n)) return PA220_PT_CORRUPT;
  if(!append(o,s,n,0,PA220_PART_GPT)) return PA220_PT_CORRUPT;
  o->parts[o->count-1].index=i+1;
  memcpy(o->parts[o->count-1].type_guid,e,16); memcpy(o->parts[o->count-1].unique_guid,e+16,16);
  memcpy(o->parts[o->count-1].name_utf16le,e+56,72);
  if(!memcmp(e+16,(uint8_t[16]){0},16)) return PA220_PT_CORRUPT;
  o->parts[o->count-1].attributes=rd64(e+48);
  for(j=0;j<i;j++) if(!memcmp(pa+j*pe+16,e+16,16)) return PA220_PT_CORRUPT;
 }
 return PA220_PT_OK;
}
int pa220_partition_scan(pa220_sector_read_fn fn,void *ctx,uint64_t capacity,struct pa220_partition_table *out){
 uint8_t m[512]; enum pa220_pt_status st;
 if(!out) return -1;
 memset(out,0,sizeof(*out)); out->capacity=capacity;
 if(!fn||capacity<2||read1(fn,ctx,0,capacity,m)<0){out->status=PA220_PT_IO;return -1;}
 if(rd16(m+510)!=MBR_SIG){out->status=PA220_PT_UNKNOWN;return -1;}
 if(m[450]==0xee) { out->format=PA220_PT_GPT; st=scan_gpt(fn,ctx,out); }
 else { out->format=PA220_PT_DOS_MBR; st=scan_mbr(fn,ctx,out,m); }
 out->status=st; if(st!=PA220_PT_OK){out->count=0;return -1;} return 0;
}
