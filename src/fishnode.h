//
// Created by cbrack02 on 10/12/26.
//

#ifndef FISHNODE_FISHNODE_H
#define FISHNODE_FISHNODE_H
#include "fish.h"
enum l2_protocol:uint8_t {
    FISHNET = 1,
    ARP = 2,
    NEIGHBOR = 3,
    DVR = 4
 };
enum arp_type:uint16_t {
    REQUEST = 1,
    RESPONSE = 2
 };
struct __attribute__((packed))l2header {
    fn_l2addr_t dest;
    fn_l2addr_t src;
    uint16_t checksum;
    uint16_t length;
    enum l2_protocol proto;
};
struct arpheader {
    enum arp_type type;
    uint16_t queried_addr;
    fn_l2addr_t l2_addr;
};
typedef struct arp_entry {
    fn_l2addr_t l2;
    fnaddr_t l3;
    time_t expire;
    uint32_t state;
    arp_resolution_cb* cb;
    intptr_t param;
    struct arp_entry *next;
}arp_entry_t;
#define L2_HEADER_LEN 0X11
#endif //FISHNODE_FISHNODE_H
