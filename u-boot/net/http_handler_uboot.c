/*
 * PA-220 read-only U-Boot state snapshot for the HTTP handler.
 * SPDX-License-Identifier: GPL-2.0+
 */
#include <common.h>
#include <net.h>
#include <net/http_handler.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

int http_handler_get_device_info(struct http_handler_info *info)
{
 struct eth_device *dev;
 u8 any=0,all_ff=0xff;
 int i;
 if(!info||!gd||!gd->bd) return -1;
 memset(info,0,sizeof(*info));
 memcpy(info->board_name,"Palo Alto Networks PA-220",
        sizeof("Palo Alto Networks PA-220"));
 memcpy(info->cpu_name,"Cavium Octeon CN7130",
        sizeof("Cavium Octeon CN7130"));
 info->board_type=20021;
 info->cpu_clock_hz=(u32)gd->cpu_clk;
 info->uptime_ms=(u32)get_timer(0);
 info->memory_start=(u64)gd->bd->bi_memstart;
 info->memory_total_bytes=(u64)gd->bd->bi_memsize;
 if(info->memory_total_bytes) info->flags|=HTTP_INFO_MEMORY_VALID;
 dev=eth_get_dev();
 if(dev) {
  info->flags|=HTTP_INFO_ETH_PRESENT;
  for(i=0;i<(int)sizeof(dev->name)&&i<(int)sizeof(info->interface_name)-1;i++) {
   info->interface_name[i]=dev->name[i];
   if(!dev->name[i]) break;
  }
  info->interface_name[sizeof(info->interface_name)-1]='\0';
  if(dev->state==ETH_STATE_ACTIVE) info->eth_state=HTTP_INFO_ETH_ACTIVE;
  else if(dev->state==ETH_STATE_PASSIVE) info->eth_state=HTTP_INFO_ETH_PASSIVE;
  else info->eth_state=HTTP_INFO_ETH_INIT;
  memcpy(info->mac,dev->enetaddr,sizeof(info->mac));
  for(i=0;i<(int)sizeof(info->mac);i++) {
   any|=info->mac[i]; all_ff&=info->mac[i];
  }
  if(any&&all_ff!=0xff&&!(info->mac[0]&1))
   info->flags|=HTTP_INFO_MAC_VALID;
 }
 if(NetOurIP) {
  memcpy(info->ip,&NetOurIP,sizeof(info->ip));
  info->flags|=HTTP_INFO_IP_VALID;
 }
 return 0;
}
