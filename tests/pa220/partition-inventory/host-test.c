#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pa220_partition.h>
#define DISK_SECTORS 256
static unsigned char disk[DISK_SECTORS][512];
static int fail_read_lba=-1;
static int rd(void *x,uint64_t l,unsigned char b[512]){(void)x;if(l>=DISK_SECTORS||(int)l==fail_read_lba)return -1;memcpy(b,disk[l],512);return 0;}
static void w32(unsigned char*p,unsigned v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
static void w64(unsigned char*p,unsigned long long v){w32(p,(unsigned)v);w32(p+4,(unsigned)(v>>32));}
static unsigned crc(const unsigned char*p,unsigned n){unsigned c=~0U,i,j;for(i=0;i<n;i++){c^=p[i];for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320U:0);}return ~c;}
static void sig(unsigned char*p){p[510]=0x55;p[511]=0xaa;}
static void clear_disk(void){memset(disk,0,sizeof(disk));}
static void mbr_entry(unsigned char*e,unsigned type,unsigned start,unsigned count){e[4]=type;w32(e+8,start);w32(e+12,count);}
static int test_mbr(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x83,10,20);if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.format!=PA220_PT_DOS_MBR||t.count!=1||t.parts[0].start_lba!=10)return 1;return 0;}
static int test_mbr_overlap(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x83,10,20);mbr_entry(disk[0]+462,0x83,20,20);if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_mbr_bad_boot(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x83,10,20);disk[0][446]=0x7f;if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_ebr(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);sig(disk[10]);mbr_entry(disk[10]+446,0x83,1,10);if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.count!=1||t.parts[0].kind!=PA220_PART_LOGICAL||t.parts[0].start_lba!=11)return 1;return 0;}
static int test_cycle(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);sig(disk[10]);mbr_entry(disk[10]+462,0x0f,0,100);if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)==0||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_ebr_outside(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);sig(disk[10]);mbr_entry(disk[10]+446,0x83,99,10);if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static void gpt_header(unsigned char*h,unsigned cur,unsigned alt,unsigned ent,unsigned ec,unsigned es,unsigned ecrc){memcpy(h,"EFI PART",8);w32(h+8,0x10000);w32(h+12,92);w64(h+24,cur);w64(h+32,alt);w64(h+40,34);w64(h+48,220);memset(h+56,0x11,16);w64(h+72,ent);w32(h+80,ec);w32(h+84,es);w32(h+88,ecrc);w32(h+16,0);w32(h+16,crc(h,92));}
static void make_gpt(void){unsigned char ent[512];clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0xee,1,255);memset(ent,0,sizeof(ent));memset(ent,0x22,16);memset(ent+16,0x33,16);w64(ent+32,40);w64(ent+40,50);memcpy(disk[2],ent,512);gpt_header(disk[1],1,255,2,4,128,crc(ent,512));memcpy(disk[254],ent,512);gpt_header(disk[255],255,1,254,4,128,crc(ent,512));}
static int test_gpt(void){struct pa220_partition_table t;make_gpt();if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.format!=PA220_PT_GPT||t.count!=1||t.parts[0].start_lba!=40)return 1;return 0;}

static void finalize_gpt(void){unsigned c=crc(disk[2],512);gpt_header(disk[1],1,255,2,4,128,c);gpt_header(disk[255],255,1,254,4,128,c);memcpy(disk[254],disk[2],512);}
static int test_gpt_overlap(void){struct pa220_partition_table t;make_gpt();memset(disk[2]+128,0x44,16);memset(disk[2]+144,0x55,16);w64(disk[2]+160,45);w64(disk[2]+168,60);finalize_gpt();if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_gpt_duplicate_guid(void){struct pa220_partition_table t;make_gpt();memset(disk[2]+128,0x44,16);memcpy(disk[2]+144,disk[2]+16,16);w64(disk[2]+160,60);w64(disk[2]+168,70);finalize_gpt();if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_gpt_outside_usable(void){struct pa220_partition_table t;make_gpt();w64(disk[2]+40,221);finalize_gpt();if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_ebr_two_node_cycle(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);sig(disk[10]);mbr_entry(disk[10]+462,0x0f,10,90);sig(disk[20]);mbr_entry(disk[20]+462,0x0f,0,100);if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}

static int test_gpt_header_crc(void){struct pa220_partition_table t;make_gpt();disk[1][16]^=1;if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_gpt_entry_crc(void){struct pa220_partition_table t;make_gpt();disk[2][80]^=1;disk[254][80]^=1;if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_gpt_backup_mismatch(void){struct pa220_partition_table t;make_gpt();disk[254][80]^=1;gpt_header(disk[255],255,1,254,4,128,crc(disk[254],512));if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_gpt_hybrid(void){struct pa220_partition_table t;make_gpt();mbr_entry(disk[0]+462,0x83,40,10);if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_AMBIGUOUS)return 1;return 0;}

struct sparse_sector { uint64_t lba; unsigned char data[512]; };
static struct sparse_sector sparse[5];
static int sparse_rd(void *x,uint64_t l,unsigned char b[512]){unsigned i;(void)x;for(i=0;i<5;i++)if(sparse[i].lba==l){memcpy(b,sparse[i].data,512);return 0;}return -1;}

static int test_ebr_read_error(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);fail_read_lba=10;if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_IO)return 1;fail_read_lba=-1;return 0;}
static int test_gpt_read_error(void){struct pa220_partition_table t;make_gpt();fail_read_lba=254;if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_IO)return 1;fail_read_lba=-1;return 0;}
static int test_gpt_backup_header_crc(void){struct pa220_partition_table t;make_gpt();disk[255][16]^=1;if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static int test_gpt_backup_lba_mismatch(void){struct pa220_partition_table t;make_gpt();gpt_header(disk[255],254,1,254,4,128,crc(disk[254],512));if(!pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.status!=PA220_PT_CORRUPT)return 1;return 0;}

static int test_pa220_live_layout(void){
 struct pa220_partition_table t; const uint64_t cap=57802752ULL, ext=16004096ULL;
 memset(sparse,0,sizeof(sparse)); sparse[0].lba=0; sig(sparse[0].data);
 mbr_entry(sparse[0].data+446,0x83,16,3952);
 mbr_entry(sparse[0].data+462,0x83,3968,8000064);
 mbr_entry(sparse[0].data+478,0x83,8004032,8000064);
 mbr_entry(sparse[0].data+494,0x0f,16004096,41798656);
 sparse[1].lba=ext; sig(sparse[1].data); mbr_entry(sparse[1].data+446,0x83,16,24000048); mbr_entry(sparse[1].data+462,0x0f,24000064,17798592);
 sparse[2].lba=40004160ULL; sig(sparse[2].data); mbr_entry(sparse[2].data+446,0x83,16,8000048); mbr_entry(sparse[2].data+462,0x0f,32000128,9798528);
 sparse[3].lba=48004224ULL; sig(sparse[3].data); mbr_entry(sparse[3].data+446,0x83,16,3952); mbr_entry(sparse[3].data+462,0x0f,32004096,9794560);
 sparse[4].lba=48008192ULL; sig(sparse[4].data); mbr_entry(sparse[4].data+446,0x82,16,9794544);
 {
  static const uint64_t starts[7]={16ULL,3968ULL,8004032ULL,16004112ULL,40004176ULL,48004240ULL,48008208ULL};
  static const uint64_t sizes[7]={3952ULL,8000064ULL,8000064ULL,24000048ULL,8000048ULL,3952ULL,9794544ULL};
  static const unsigned char types[7]={0x83,0x83,0x83,0x83,0x83,0x83,0x82};
  unsigned i;
  if(pa220_partition_scan(sparse_rd,0,cap,&t)||t.format!=PA220_PT_DOS_MBR||t.count!=7) return 1;
  for(i=0;i<7;i++){
   if(t.parts[i].start_lba!=starts[i]||t.parts[i].sectors!=sizes[i]||t.parts[i].type!=types[i]) return 1;
   if(t.parts[i].kind!=(i<3?PA220_PART_PRIMARY:PA220_PART_LOGICAL)) return 1;
  }
 }
 return 0;
}

int main(void){int e=0;e+=test_mbr();e+=test_mbr_overlap();e+=test_mbr_bad_boot();e+=test_ebr();e+=test_cycle();e+=test_ebr_outside();e+=test_ebr_two_node_cycle();e+=test_gpt();e+=test_gpt_overlap();e+=test_gpt_duplicate_guid();e+=test_gpt_outside_usable();e+=test_gpt_header_crc();e+=test_gpt_entry_crc();e+=test_gpt_backup_mismatch();e+=test_gpt_hybrid();e+=test_ebr_read_error();e+=test_gpt_read_error();e+=test_gpt_backup_header_crc();e+=test_gpt_backup_lba_mismatch();e+=test_pa220_live_layout();if(e){printf("partition inventory failures=%d\n",e);return 1;}puts("PA220_PARTITION_INVENTORY_HOST=PASS");return 0;}
