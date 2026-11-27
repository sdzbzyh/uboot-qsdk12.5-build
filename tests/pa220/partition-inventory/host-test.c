#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pa220_partition.h>
#define DISK_SECTORS 256
static unsigned char disk[DISK_SECTORS][512];
static int rd(void *x,uint64_t l,unsigned char b[512]){(void)x;if(l>=DISK_SECTORS)return -1;memcpy(b,disk[l],512);return 0;}
static void w32(unsigned char*p,unsigned v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
static void w64(unsigned char*p,unsigned long long v){w32(p,(unsigned)v);w32(p+4,(unsigned)(v>>32));}
static unsigned crc(const unsigned char*p,unsigned n){unsigned c=~0U,i,j;for(i=0;i<n;i++){c^=p[i];for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320U:0);}return ~c;}
static void sig(unsigned char*p){p[510]=0x55;p[511]=0xaa;}
static void clear_disk(void){memset(disk,0,sizeof(disk));}
static void mbr_entry(unsigned char*e,unsigned type,unsigned start,unsigned count){e[4]=type;w32(e+8,start);w32(e+12,count);}
static int test_mbr(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x83,10,20);if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.format!=PA220_PT_DOS_MBR||t.count!=1||t.parts[0].start_lba!=10)return 1;return 0;}
static int test_ebr(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);sig(disk[10]);mbr_entry(disk[10]+446,0x83,1,10);if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.count!=1||t.parts[0].kind!=PA220_PART_LOGICAL||t.parts[0].start_lba!=11)return 1;return 0;}
static int test_cycle(void){struct pa220_partition_table t;clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0x0f,10,100);sig(disk[10]);mbr_entry(disk[10]+462,0x0f,0,100);if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)==0||t.status!=PA220_PT_CORRUPT)return 1;return 0;}
static void gpt_header(unsigned char*h,unsigned cur,unsigned alt,unsigned ent,unsigned ec,unsigned es,unsigned ecrc){memcpy(h,"EFI PART",8);w32(h+8,0x10000);w32(h+12,92);w64(h+24,cur);w64(h+32,alt);w64(h+40,34);w64(h+48,220);memset(h+56,0x11,16);w64(h+72,ent);w32(h+80,ec);w32(h+84,es);w32(h+88,ecrc);w32(h+16,0);w32(h+16,crc(h,92));}
static int test_gpt(void){struct pa220_partition_table t;unsigned char ent[512];clear_disk();sig(disk[0]);mbr_entry(disk[0]+446,0xee,1,255);memset(ent,0,sizeof(ent));memset(ent,0x22,16);memset(ent+16,0x33,16);w64(ent+32,40);w64(ent+40,50);memcpy(disk[2],ent,512);gpt_header(disk[1],1,255,2,4,128,crc(ent,512));memcpy(disk[254],ent,512);gpt_header(disk[255],255,1,254,4,128,crc(ent,512));if(pa220_partition_scan(rd,0,DISK_SECTORS,&t)||t.format!=PA220_PT_GPT||t.count!=1||t.parts[0].start_lba!=40)return 1;return 0;}
int main(void){int e=0;e+=test_mbr();e+=test_ebr();e+=test_cycle();e+=test_gpt();if(e){printf("partition inventory failures=%d\n",e);return 1;}puts("PA220_PARTITION_INVENTORY_HOST=PASS");return 0;}
