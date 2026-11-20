#include <stdio.h>
#include <string.h>
#include <net/tcp.h>

static int passes, failures;
static unsigned emitted;
static u8 last_packet[1600];
static u16 last_len;

static const u8 server_ip[4] = {10, 0, 0, 1};
static const u8 client_ip[4] = {10, 0, 0, 2};

#define CHECK(x) do { \
    if (!(x)) { \
        printf("FAIL line=%d: %s\n", __LINE__, #x); \
        failures++; \
    } else { \
        passes++; \
    } \
} while (0)

static void put16(u8 *p, u16 v)
{
    p[0] = (u8)(v >> 8);
    p[1] = (u8)v;
}

static void put32(u8 *p, u32 v)
{
    p[0] = (u8)(v >> 24);
    p[1] = (u8)(v >> 16);
    p[2] = (u8)(v >> 8);
    p[3] = (u8)v;
}

static u32 get32(const u8 *p)
{
    return ((u32)p[0] << 24) |
           ((u32)p[1] << 16) |
           ((u32)p[2] << 8) | p[3];
}

static int emit_cb(void *arg, const u8 src[4], const u8 dst[4],
                   const u8 *packet, u16 len)
{
    (void)arg;
    (void)src;
    (void)dst;

    if (len > sizeof(last_packet))
        return -1;

    memcpy(last_packet, packet, len);
    last_len = len;
    emitted++;
    return 0;
}

static int input(struct tcp_core_connection *c, u32 seq,
                 u32 ack, u8 flags, const u8 *data,
                 u16 len, u32 now)
{
    u8 packet[1600];
    u16 plen;

    memset(packet, 0, sizeof(packet));
    put16(packet, 40000);
    put16(packet + 2, 8080);
    put32(packet + 4, seq);
    put32(packet + 8, ack);
    packet[12] = 0x50;
    packet[13] = flags;
    put16(packet + 14, 4096);

    if (len)
        memcpy(packet + 20, data, len);

    plen = (u16)(20 + len);
    put16(packet + 16,
          tcp_checksum_ipv4(client_ip, server_ip, packet, plen));

    return tcp_core_input(c, client_ip, server_ip,
                          packet, plen, now, emit_cb, NULL);
}

static void handshake(struct tcp_core_connection *c)
{
    tcp_core_init(c);

    CHECK(tcp_core_listen(c, server_ip, 8080, 1000) == 0);

    CHECK(input(c, 5000, 0, TCP_FLAG_SYN, NULL, 0, 1) == 1);
    CHECK(c->state == TCP_CORE_SYN_RECEIVED);

    CHECK(input(c, 5001, 1001, TCP_FLAG_ACK, NULL, 0, 2) == 1);
    CHECK(c->state == TCP_CORE_ESTABLISHED);
    CHECK(c->rcv_nxt == 5001);
}

static void test_close_wait_post_fin_data(void)
{
    struct tcp_core_connection c;
    int rc;

    printf("\n[CLOSE-WAIT POST-FIN DATA]\n");
    handshake(&c);

    rc = input(&c, 5001, 1001,
               TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0, 10);
    CHECK(rc == 0);
    CHECK(c->state == TCP_CORE_CLOSE_WAIT);
    CHECK(c->fin_received == 1);
    CHECK(c->rcv_nxt == 5002);

    rc = input(&c, 5002, 1001,
               TCP_FLAG_ACK | TCP_FLAG_PSH,
               (const u8 *)"X", 1, 11);

    CHECK(rc == 0);

    printf("POST_FIN_INPUT_RC=%d\n", rc);
    printf("POST_FIN_STATE=%d RX_LEN=%u RCV_NXT=%u\n",
           c.state, c.rx_len, c.rcv_nxt);

    CHECK(c.rx_len == 0);
    CHECK(c.rcv_nxt == 5002);

    if (c.rx_len == 0 && c.rcv_nxt == 5002)
        printf("RESULT: POST_FIN_DATA_REJECTED\n");
    else
        printf("RESULT: POST_FIN_DATA_ACCEPTED (PROTOCOL DEFECT)\n");
}

static void test_time_wait_duplicate_fin(void)
{
    struct tcp_core_connection c;
    u32 original_deadline;
    unsigned before;
    int rc;

    printf("\n[TIME-WAIT DUPLICATE FIN]\n");
    handshake(&c);

    CHECK(tcp_core_close(&c, 10, emit_cb, NULL) == 0);
    CHECK(c.state == TCP_CORE_FIN_WAIT_1);

    rc = input(&c, 5001, 1002, TCP_FLAG_ACK, NULL, 0, 11);
    CHECK(rc == 0);
    CHECK(c.state == TCP_CORE_FIN_WAIT_2);

    rc = input(&c, 5001, 1002,
               TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0, 12);
    CHECK(rc == 0);
    CHECK(c.state == TCP_CORE_TIME_WAIT);

    original_deadline = c.time_wait_deadline;
    before = emitted;

    rc = input(&c, 5001, 1002,
               TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0, 20000);

    CHECK(rc == 0);
    CHECK(emitted == before + 1);
    CHECK(last_len >= 20);
    CHECK((last_packet[13] & TCP_FLAG_ACK) != 0);
    CHECK(get32(last_packet + 8) == 5002);
    CHECK(c.time_wait_deadline == original_deadline);

    printf("DUPLICATE_FIN_INPUT_RC=%d\n", rc);
    printf("ORIGINAL_DEADLINE=%u\n", original_deadline);
    printf("AFTER_DUPLICATE_FIN=%u\n", c.time_wait_deadline);

    if (c.time_wait_deadline == original_deadline)
        printf("RESULT: TIME_WAIT_DEADLINE_PRESERVED\n");
    else
        printf("RESULT: TIME_WAIT_DEADLINE_RESTARTED\n");
}

int main(void)
{
    printf("PA-220 TCP FOCUSED PROTOCOL RETEST\n");

    test_close_wait_post_fin_data();
    test_time_wait_duplicate_fin();

    printf("\nASSERTIONS_PASSED=%d\n", passes);
    printf("ASSERTIONS_FAILED=%d\n", failures);
    printf("TEST_RC=%d\n", failures ? 1 : 0);

    return failures ? 1 : 0;
}
