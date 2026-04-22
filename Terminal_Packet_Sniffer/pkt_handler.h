#include "general.h"

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80

void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

void display_filter(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

void print_l7(const u_char *packet, int caplen, int len_to_skip, int dst_port);

void print_l4_tcp(const u_char *packet, int caplen, int len_to_skip);

void print_l4_udp(const u_char *packet, int caplen, int len_to_skip);

void print_l3_arp(const u_char *packet, int caplen, int len_to_skip);

void print_l3_ipv6(const u_char *packet, int caplen, int len_to_skip);

void print_l3_ipv4(const u_char *packet, int caplen, int len_to_skip);

void print_l2(const u_char *packet, int caplen);