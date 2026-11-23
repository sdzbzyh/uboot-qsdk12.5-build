#ifndef PA220_HOST_NET_H
#define PA220_HOST_NET_H
#include <common.h>
typedef u32 IPaddr_t;
struct bd_info;
struct eth_device {
 char name[16];
 u8 enetaddr[6];
 int state;
};
#define ETH_STATE_ACTIVE 2
#define ETH_STATE_PASSIVE 1
extern IPaddr_t NetOurIP;
void net_init(void);
void eth_halt(void);
void eth_set_current(void);
struct eth_device *eth_get_dev(void);
int eth_init(struct bd_info *bd);
void eth_rx(void);
void net_set_tcp_handler(void (*handler)(const uchar *, unsigned,
 IPaddr_t, IPaddr_t, const uchar [6]));
int ctrlc(void);
u32 get_timer(u32 base);
#endif
