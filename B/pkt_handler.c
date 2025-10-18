#include <net/if_arp.h>
#include "general.h"
#include "pkt_handler.h"

void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet){
    // Add packet to be process in the sniff_log array
    snprintf(sniff_log[pkt_num].timestamp, sizeof(sniff_log[pkt_num].timestamp),
             "%02ld:%02ld:%02ld.%06ld", (header->ts.tv_sec / 3600) % 24, (header->ts.tv_sec / 60) % 60, 
             header->ts.tv_sec % 60, header->ts.tv_usec);
    
    sniff_log[pkt_num].frame_length = header->len;
    sniff_log[pkt_num].captured_length = header->caplen;
    sniff_log[pkt_num].packet = (u_char *)malloc(header->caplen);   // Allocating space for the packet
    memcpy(sniff_log[pkt_num].packet, packet, header->caplen);

    // Incrementing since packet number to be printed starts from 1
    pkt_num++;

    // Printing the packet metadata
    printf("-----------------------------------------\n");
    printf("Packet #%d | Timestamp: %ld.%.06ld | Length: %d bytes\n", pkt_num, header->ts.tv_sec, header->ts.tv_usec, header->caplen);
   
    /*
    // Printing the first 16 raw bytes of the packet frame in hexadecimal format (or entire packet if packet is smaller than 16 bytes) 
    int i;
    int printlen = header->caplen < 16 ? header->caplen : 16;
    for (i = 0; i < printlen; i++){
        printf("%02X ", packet[i]);
    }
    printf("\n\n");
    */

    print_l2(packet, header->caplen);
}

void display_filter(u_char *user, const struct pcap_pkthdr *header, const u_char *packet){
    if(selected_proto == 1){    // HTTP
        struct ethhdr *eth = (struct ethhdr *)packet;
        uint16_t ether_type = ntohs(eth->h_proto);

        int ipv4 = 0;
        int ipv6 = 0;

        if(ether_type == ETH_P_IP){
            ipv4 = 1;
        }
        else if(ether_type == ETH_P_IPV6){
            ipv6 = 1;
        }

        if(ipv4){
            struct iphdr *iph = (struct iphdr *)(packet + ETH_HLEN);
            if(iph->protocol == 6){
                struct tcphdr *tcph = (struct tcphdr *)(packet + ETH_HLEN + iph->ihl * 4);
                unsigned short dst = ntohs(tcph->dest);
                if(dst == 80){
                    packet_handler(user, header, packet);
                }
            }
            else if(iph->protocol == 17){
                struct udphdr *udph = (struct udphdr *)(packet + ETH_HLEN + iph->ihl * 4);
                if(ntohs(udph->source) == 80){
                    packet_handler(user, header, packet);
                }
            }
        }

        else if(ipv6){
            struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + ETH_HLEN);
            if(ip6h->ip6_nxt == 6){
                struct tcphdr *tcph = (struct tcphdr *)(packet + ETH_HLEN + sizeof(struct ip6_hdr));
                unsigned short dst = ntohs(tcph->dest);
                if(dst == 80){
                    packet_handler(user, header, packet);
                }
            }
            else if(ip6h->ip6_nxt == 17){
                struct udphdr *udph = (struct udphdr *)(packet + ETH_HLEN + sizeof(struct ip6_hdr));
                if(ntohs(udph->source) == 80){
                    packet_handler(user, header, packet);
                }
            }
        }
    }

    if(selected_proto == 2){    // HTTPS
        struct ethhdr *eth = (struct ethhdr *)packet;
        uint16_t ether_type = ntohs(eth->h_proto);

        int ipv4 = 0;
        int ipv6 = 0;

        if(ether_type == ETH_P_IP){
            ipv4 = 1;
        }
        else if(ether_type == ETH_P_IPV6){
            ipv6 = 1;
        }

        if(ipv4){
            struct iphdr *iph = (struct iphdr *)(packet + ETH_HLEN);
            if(iph->protocol == 6){
                struct tcphdr *tcph = (struct tcphdr *)(packet + ETH_HLEN + iph->ihl * 4);
                unsigned short dst = ntohs(tcph->dest);
                if(dst == 443){
                    packet_handler(user, header, packet);
                }
            }
            else if(iph->protocol == 17){
                struct udphdr *udph = (struct udphdr *)(packet + ETH_HLEN + iph->ihl * 4);
                if(ntohs(udph->source) == 443){
                    packet_handler(user, header, packet);
                }
            }
        }

        else if(ipv6){
            struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + ETH_HLEN);
            if(ip6h->ip6_nxt == 6){
                struct tcphdr *tcph = (struct tcphdr *)(packet + ETH_HLEN + sizeof(struct ip6_hdr));
                unsigned short dst = ntohs(tcph->dest);
                if(dst == 443){
                    packet_handler(user, header, packet);
                }
            }
            else if(ip6h->ip6_nxt == 17){
                struct udphdr *udph = (struct udphdr *)(packet + ETH_HLEN + sizeof(struct ip6_hdr));
                if(ntohs(udph->source) == 443){
                    packet_handler(user, header, packet);
                }
            }
        }
    }

    if(selected_proto == 3){    // DNS
        struct ethhdr *eth = (struct ethhdr *)packet;
        uint16_t ether_type = ntohs(eth->h_proto);

        int ipv4 = 0;
        int ipv6 = 0;

        if(ether_type == ETH_P_IP){
            ipv4 = 1;
        }
        else if(ether_type == ETH_P_IPV6){
            ipv6 = 1;
        }

        if(ipv4){
            struct iphdr *iph = (struct iphdr *)(packet + ETH_HLEN);
            if(iph->protocol == 6){
                struct tcphdr *tcph = (struct tcphdr *)(packet + ETH_HLEN + iph->ihl * 4);
                unsigned short dst = ntohs(tcph->dest);
                if(dst == 53){
                    packet_handler(user, header, packet);
                }
            }
            else if(iph->protocol == 17){
                struct udphdr *udph = (struct udphdr *)(packet + ETH_HLEN + iph->ihl * 4);
                if(ntohs(udph->source) == 53){
                    packet_handler(user, header, packet);
                }
            }
        }

        else if(ipv6){
            struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + ETH_HLEN);
            if(ip6h->ip6_nxt == 6){
                struct tcphdr *tcph = (struct tcphdr *)(packet + ETH_HLEN + sizeof(struct ip6_hdr));
                unsigned short dst = ntohs(tcph->dest);
                if(dst == 53){
                    packet_handler(user, header, packet);
                }
            }
            else if(ip6h->ip6_nxt == 17){
                struct udphdr *udph = (struct udphdr *)(packet + ETH_HLEN + sizeof(struct ip6_hdr));
                if(ntohs(udph->source) == 53){
                    packet_handler(user, header, packet);
                }
            }
        }
    }

    if(selected_proto == 4){    // ARP
        struct ethhdr *eth = (struct ethhdr *)packet;
        uint16_t ether_type = ntohs(eth->h_proto);

        if(ether_type == ETH_P_ARP){
            packet_handler(user, header, packet);
        }
    }

    if(selected_proto == 5){    // TCP
        // L3 = IPv4 or IPv6 then check for TCP
        struct ethhdr *eth = (struct ethhdr *)packet;
        uint16_t ether_type = ntohs(eth->h_proto);

        int ipv4 = 0;
        int ipv6 = 0;

        if(ether_type == ETH_P_IP){
            ipv4 = 1;
        }
        else if(ether_type == ETH_P_IPV6){
            ipv6 = 1;
        }

        if(ipv4){
            struct iphdr *iph = (struct iphdr *)(packet + ETH_HLEN);
            if(iph->protocol == 6){
                packet_handler(user, header, packet);
            }
        }

        else if(ipv6){
            struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + ETH_HLEN);
            if(ip6h->ip6_nxt == 6){
                packet_handler(user, header, packet);
            }
        }
    }
    
    if(selected_proto == 6){    // UDP
        // L3 = IPv4 or IPv6 then check for TCP
        struct ethhdr *eth = (struct ethhdr *)packet;
        uint16_t ether_type = ntohs(eth->h_proto);

        int ipv4 = 0;
        int ipv6 = 0;

        if(ether_type == ETH_P_IP){
            ipv4 = 1;
        }
        else if(ether_type == ETH_P_IPV6){
            ipv6 = 1;
        }

        if(ipv4){
            struct iphdr *iph = (struct iphdr *)(packet + ETH_HLEN);
            if(iph->protocol == 17){
                packet_handler(user, header, packet);
            }
        }

        else if(ipv6){
            struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + ETH_HLEN);
            if(ip6h->ip6_nxt == 17){
                packet_handler(user, header, packet);
            }
        }
    }
}

void print_l7(const u_char *packet, int caplen, int len_to_skip, int dst_port){
    // Printing initial line without length
    printf("L7 (Payload): ");
    if(dst_port == 80){
        printf("Identified as HTTP on port 80 - ");
    }
    else if(dst_port == 443){
        printf("Identified as HTTPS/TLS on port 443 - ");
    }
    else if(dst_port == 53){
        printf("Identified as DNS on port 53 - ");
    }
    else{
        printf("Unknown protocol on port %d - ", dst_port);
    }
    int payload_len = caplen - len_to_skip;
    printf("%d bytes\n", payload_len);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          

    int print_len = payload_len <= 64 ? payload_len : 64;

    printf("Data (first %d bytes):\n", print_len);

    // Print hex bytes (16 bytes per line)
    for(int i = 0; i < print_len; i++){
        printf("%02X ", packet[len_to_skip + i]);
        if((i+1) % 16 == 0){        // Adjacently print ASCII interpretation
            for(int j = i-15; j <= i; j++){
                unsigned char c = packet[len_to_skip + j];
                if(c >= 32 && c <= 126) printf("%c", c);
                else printf(".");
            }
            printf("\n");
        }
    }
    if(print_len % 16 != 0){
        for(int i = 0; i < 16 - (print_len % 16); i++){
            printf("   ");
        }

        int div = print_len / 16;
        for(int i = 0; i < print_len % 16; i++){
            unsigned char c = packet[len_to_skip + div * 16 + i];
            if(c >= 32 && c <= 126) printf("%c", c);
            else printf(".");
        }
        printf("\n");
    }

    /*
    // Print ASCII interpretation
    for(int i = 0; i < print_len; i++){
        unsigned char c = packet[len_to_skip + i];
        if(c >= 32 && c <= 126) printf("%c", c);
        else printf(".");

        if((i+1) % 16 == 0) printf("\t");
    }
    printf("\n");
    */
}

void print_l4_tcp(const u_char *packet, int caplen, int len_to_skip){
    // Printing layer 4 information
    struct tcphdr *tcph = (struct tcphdr *)(packet + len_to_skip);

    printf("L4 (TCP): ");

    unsigned short src = ntohs(tcph->source);
    unsigned short dst = ntohs(tcph->dest);

    printf("Src Port: %u ", src);
    if(src == 80){
        printf("(HTTP) | ");
    }
    else if(src == 443){
        printf("(HTTPS) | ");
    }
    else if(src == 53){
        printf("DNS | ");
    }
    else{
        printf("| ");
    }

    printf("Dst Port: %u ", dst);
    if(dst == 80){
        printf("(HTTP) | ");
    }
    else if(dst == 443){
        printf("(HTTPS) | ");
    }
    else if(dst == 53){
        printf("DNS | ");
    }
    else{
        printf("| ");
    }

    printf("Seq: %u | Ack: %u | Flags: ", ntohl(tcph->seq), ntohl(tcph->ack_seq));

    int flag = 0;
    unsigned char flags = tcph->th_flags;
    if(flags & TH_CWR){
        printf("[CWR] ");
        flag = 1;
    }
    if(flags & TH_ECE){
        printf("[ECE] "); 
        flag = 1;
    }
    if(flags & TH_URG){
        printf("[URG] ");
        flag = 1;
    }
    if(flags & TH_ACK){
        printf("[ACK] ");
        flag = 1;
    }
    if(flags & TH_PUSH){
        printf("[PSH] ");
        flag = 1;
    }
    if(flags & TH_RST){
        printf("[RST] ");
        flag = 1;
    }
    if(flags & TH_SYN){
        printf("[SYN] ");
        flag = 1;
    }
    if(flags & TH_FIN){
        printf("[FIN] ");
        flag = 1;
    }
    if(flag == 0){
        printf("None");
    }
    printf("\n");
    
    printf("Window: %d | Checksum: %#04X | Header Length: %d bytes\n", ntohs(tcph->window), ntohs(tcph->check), (unsigned int)tcph->doff * 4);

    print_l7(packet, caplen, len_to_skip + tcph->doff * 4, dst);
}

void print_l4_udp(const u_char *packet, int caplen, int len_to_skip){
    // Retrieving and printing layer 4 information
    struct udphdr *udph = (struct udphdr *)(packet + len_to_skip);

    printf("L4 (UDP): Src Port: %d ", ntohs(udph->source));
    if(ntohs(udph->source) == 80){
        printf("(HTTP) | ");
    }
    else if(ntohs(udph->source) == 443){
        printf("(HTTPS) | ");
    }
    else if(ntohs(udph->source) == 53){
        printf("DNS | ");
    }
    else{
        printf("| ");
    }

    printf("Dst Port: %d ", ntohs(udph->dest));
    if(ntohs(udph->dest) == 80){
        printf("(HTTP) | ");
    }
    else if(ntohs(udph->dest) == 443){
        printf("(HTTPS) | ");
    }
    else if(ntohs(udph->dest) == 53){
        printf("DNS | ");
    }
    else{
        printf("| ");
    }

    printf("Length: %d | Checksum: %#04X\n", ntohs(udph->len), ntohs(udph->check));

    print_l7(packet, caplen, len_to_skip + sizeof(struct udphdr), ntohs(udph->dest));
}

void print_l3_ipv4(const u_char *packet, int caplen, int len_to_skip){
    // Retrieving layer 3 information
    struct iphdr *iph = (struct iphdr *)(packet + len_to_skip);      // Skipping Ethernet header data
    
    // Printing layer 3 information
    char protocol_name[10];
    if(iph->protocol == 6){
        strcpy(protocol_name, "TCP");
    }
    else if(iph->protocol == 17){
        strcpy(protocol_name, "UDP");
    }
    else{
        strcpy(protocol_name, "Unknown");
    }

    struct in_addr src_addr, dst_addr;
    src_addr.s_addr = iph->saddr;
    dst_addr.s_addr = iph->daddr;

    printf("L3 (IPv4): Src IP: %s | Dst IP: %s | Protocol: %s (%u) | TTL: %u\n", 
        inet_ntoa(src_addr), inet_ntoa(dst_addr), protocol_name, iph->protocol, iph->ttl);

    printf("ID: 0x%X | Total Length: %u | Header Length: %u bytes | ",
        ntohs(iph->id), ntohs(iph->tot_len), iph->ihl * 4);
    
    // Decoding the flags
    uint16_t frag_off = ntohs(iph->frag_off);
    int df = (frag_off & 0x4000) >> 14;
    int mf = (frag_off & 0x2000) >> 13;
    printf("Flags: ");
    if(df) printf("DF ");
    if(mf) printf("MF ");
    if(!df && !mf) printf("None");
    printf("\n");

    if(iph->protocol == 6){
        print_l4_tcp(packet, caplen, len_to_skip + iph->ihl * 4);
    }
    else if(iph->protocol == 17){
        print_l4_udp(packet, caplen, len_to_skip + iph->ihl * 4);
    }
}

void print_l3_ipv6(const u_char *packet, int caplen, int len_to_skip){
    // Retrieving layer 3 information
    struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + len_to_skip);      // Skipping Ethernet header data
    
    char src_addr[INET6_ADDRSTRLEN];
    char dst_addr[INET6_ADDRSTRLEN];

    // Converting the addresses to human readable form
    inet_ntop(AF_INET6, &(ip6h->ip6_src), src_addr, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &(ip6h->ip6_dst), dst_addr, INET6_ADDRSTRLEN);

    // Retrieving data to print
    uint8_t next_header = ip6h->ip6_nxt;
    char nxt_hdr_name[10];
    if(next_header == 6){
        strcpy(nxt_hdr_name, "TCP");
    }
    else if(next_header == 17){
        strcpy(nxt_hdr_name, "UDP");
    }
    else{
        strcpy(nxt_hdr_name, "Unknown");
    }
    uint8_t hop_limit = ip6h->ip6_hlim;

    uint32_t vtcfl = ntohl(*(uint32_t *)ip6h);          // Reading first 4 bytes of the header (32 bits)
    uint8_t traffic_class = (vtcfl & 0x0ff00000) >> 20; // Traffic class is formed by first 4 bits + 8 bits
    uint32_t flow_label = vtcfl & 0x000fffff;           // Next 20 bits form the flow label

    uint16_t payload_len = ntohs(ip6h->ip6_plen);

    // Printing layer 3 information
    printf("L3 (IPv6): Src IP: %s | Dst IP : %s\n", src_addr, dst_addr);
    printf("Next Header: %s (%u) | Hop Limit: %u | Traffic Class: %u | Flow Label: 0x%05X | Payload Length: %u\n",
        nxt_hdr_name, next_header, hop_limit, traffic_class, flow_label, payload_len);

    if(next_header == 6){
        print_l4_tcp(packet, caplen, len_to_skip + sizeof(struct ip6_hdr));
    }
    else if(next_header == 17){
        print_l4_udp(packet, caplen, len_to_skip + sizeof(struct ip6_hdr));
    }
}

void print_l3_arp(const u_char *packet, int caplen, int len_to_skip){
    // Retrieving layer 3 information
    struct ether_arp *arp = (struct ether_arp *)(packet + len_to_skip);     // Skipping Ethernet header data

    // Retrieving data to print
    uint16_t op = ntohs(arp->ea_hdr.ar_op);
    char operation[10];
    if(op == ARPOP_REQUEST){
        strcpy(operation, "Request");
    }
    else if(op == ARPOP_REPLY){
        strcpy(operation, "Reply");
    }
    else{
        strcpy(operation, "Unknown");
    }

    printf("L3 (ARP): Operation: %s (%d) | Sender IP: %s | Target IP: %s\n", 
        operation, op, inet_ntoa(*(struct in_addr *)arp->arp_spa), inet_ntoa(*(struct in_addr *)arp->arp_tpa));
 
    printf("Sender MAC: %02X:%02X:%02X:%02X:%02X:%02X | Target MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
        arp->arp_sha[0], arp->arp_sha[1], arp->arp_sha[2], arp->arp_sha[3], arp->arp_sha[4], arp->arp_sha[5],
        arp->arp_tha[0], arp->arp_tha[1], arp->arp_tha[2], arp->arp_tha[3], arp->arp_tha[4], arp->arp_tha[5]);

    printf("HW Type: %u | Proto Type: 0x%04X | HW Len: %u | Proto Len: %u\n", 
        ntohs(arp->ea_hdr.ar_hrd), ntohs(arp->ea_hdr.ar_pro), arp->ea_hdr.ar_hln, arp->ea_hdr.ar_pln);
}

void print_l2(const u_char *packet, int caplen){
    // reading relevant data from the ethernet header
    struct ethhdr *eth = (struct ethhdr *)packet;
    printf("L2 (Ethernet): Dst MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X | ", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf("Src MAC: %.2X-%.2X-%.2X-%.2X-%.2X-%.2X | ", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);

    uint16_t ether_type = ntohs(eth->h_proto);
    
    // printing the ethertype
    if(ether_type == ETH_P_IP){
        printf("EtherType: IPv4 (0x0800)\n");
        print_l3_ipv4(packet, caplen, ETH_HLEN);
    }
    else if(ether_type == ETH_P_IPV6){
        printf("EtherType: IPv6 (0x86DD)\n");
        print_l3_ipv6(packet, caplen, ETH_HLEN);
    }
    else if(ether_type == ETH_P_ARP){
        printf("EtherType: ARP (0x0806)\n");
        print_l3_arp(packet, caplen, ETH_HLEN);
    }
    else{
        printf("EtherType: Unknown\n");
    }
}