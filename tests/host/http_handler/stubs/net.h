#ifndef __HTTP_HANDLER_TEST_NET_H__
#define __HTTP_HANDLER_TEST_NET_H__
#include <linux/types.h>
enum eth_state_t { ETH_STATE_INIT, ETH_STATE_PASSIVE, ETH_STATE_ACTIVE };
struct eth_device { char name[16]; unsigned char enetaddr[6]; int state; };
extern struct eth_device *http_test_eth_current;
static inline struct eth_device *eth_get_dev(void)
{
 return http_test_eth_current;
}
extern u32 NetOurIP;
#endif
