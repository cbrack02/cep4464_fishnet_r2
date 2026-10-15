//
// Created by cbrack02 on 10/12/26.
//

#ifndef FISHNODE_FISHNODE_H
#define FISHNODE_FISHNODE_H
#include "fish.h"
#define EXPIRE_TIME 180
void my_send_arp_response(fn_l2addr_t l2addr, fnaddr_t l3addr);
void my_resolve_fnaddr(fnaddr_t addr, arp_resolution_cb cb, void *param);
void my_arp_cb(fn_l2addr_t l2addr, void* l2frame);
struct arp_entry* find_arp_entry(fnaddr_t addr);
void arp_invalidate_entry(void* entry);
enum l2_protocol:uint8_t {
    FISHNET = 1,
    ARP = 2,
    NEIGHBOR = 3,
    DVR = 4
 };
enum arp_type:uint32_t {
    REQUEST = 1,
    RESPONSE = 2
 };
enum arp_state:uint16_t {
    RESOLVED = 1,
    INCOMPLETE = 2,
    DOWN = 3
 };
struct __attribute__((packed))l2header {
    fn_l2addr_t dest;
    fn_l2addr_t src;
    uint16_t checksum;
    uint16_t length;
    enum l2_protocol proto;
};
struct __attribute__((packed))fishheader{
    uint8_t ttl;
    uint8_t proto;
    uint32_t packet_id;
    fnaddr_t src;
    fnaddr_t dest;
};
struct __attribute__((packed))arpheader {
    enum arp_type type;
    fnaddr_t queried_l3;
    fn_l2addr_t l2;
};
typedef struct arp_entry {
    fn_l2addr_t l2;
    fnaddr_t l3;
    time_t expire;
    enum arp_state state;
    struct arp_cb* cb;
    void* param;
    struct arp_entry *next;
    event monitor;
}arp_entry_t;
struct arp_cb {
    arp_resolution_cb cb;
    void* packet;
    struct arp_cb* next;
};
struct ACParam {
    fnaddr_t l3;
    fn_l2addr_t l2;
    int to;
    int success;
    int start_time;
    int retry_cnt;
    int called;
    int addit;
    fnaddr_t qL3;
    int schedule;
};
#define L2_HEADER_LEN 0X11
#endif //FISHNODE_FISHNODE_H
