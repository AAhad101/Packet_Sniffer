#ifndef GENERAL_H
#define GENERAL_H

#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <netinet/if_ether.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_PACKETS 10000

extern pcap_t *handle;
extern int pkt_num;
extern int selected_proto;
extern int session_occur;
extern int ctrlc;

typedef unsigned char u_char;

typedef struct PacketRecord{
    char timestamp[20];
    int frame_length;
    int captured_length;
    u_char *packet;
} PacketRecord;

extern PacketRecord sniff_log[MAX_PACKETS];

pcap_if_t *find_interface(pcap_if_t *alldevs, int index);

#endif // GENERAL_H