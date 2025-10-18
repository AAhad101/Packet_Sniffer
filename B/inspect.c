#include "inspect.h"
#include "pkt_handler.h"

// Printing all packets of the previous session
void print_session(){
    for(int i = 0; i < pkt_num; i++){
        printf("-----------------------------------------\n");
        PacketRecord cur_pkt = sniff_log[i];
        printf("Packet #%d | Timestamp: %s | Length: %d bytes\n", i + 1, cur_pkt.timestamp, cur_pkt.captured_length);
        print_l2(cur_pkt.packet, cur_pkt.captured_length);
    }
}

void inspect_packet(int analysed_num){
    int index = analysed_num - 1;
    PacketRecord cur_packet = sniff_log[index];

    u_char *packet = cur_packet.packet;
    int caplen = cur_packet.captured_length;
    //int frame_len = cur_packet.frame_length;
    
    int len_to_skip = 0;
    int ipv4 = 0;
    int ipv6 = 0;
    int arp = 0;
    int protocol = 0;

    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                       C-SHARK DETAILED PACKET ANALYSIS                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Packeting summary
    printf("PACKET SUMMARY\n");
    printf("══════════════\n");
    printf("Packet Number: %d\n", analysed_num);
    printf("Timestamp: %s\n", cur_packet.timestamp);
    printf("Frame Length: %d\n", cur_packet.frame_length);
    printf("Captured Length: %d\n", cur_packet.captured_length);
    printf("\n");

    // Printing the entire frame hex dump
    printf("COMPLETE FRAME HEX DUMP\n");
    printf("═══════════════════════\n");

    printf("┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐\t");
    printf("┌────────────────┐\n");
    for(int i = 0; i < caplen; i++){
        printf("│%02X", packet[i]);
        if((i+1) % 16 == 0){        // Adjacently print ASCII interpretation
            printf("│\t│");
            for(int j = i-15; j <= i; j++){
                unsigned char c = packet[j];
                if(c >= 32 && c <= 126) printf("%c", c);
                else printf(".");
            }
            printf("│\n");
        }
    }
    if(caplen % 16 != 0){       // Padding the last row accordingly
        for(int i = 0; i < 16 - (caplen % 16); i++){
            printf("│  ");
        }
        printf("│\t│"); 
        
        int div = caplen / 16;
        for(int i = 0; i < caplen % 16; i++){
            unsigned char c = packet[div * 16 + i];
            if(c >= 32 && c <= 126) printf("%c", c);
            else printf(".");
        }
        int space_num = 16 - caplen % 16;
        for(int i = 0; i < space_num; i++){
            printf(" ");
        }
        printf("│\n");
    }
    printf("└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘\t");
    printf("└────────────────┘\n");
    printf("\n");

    // Printing the layer-by-layer analysis
    printf("LAYER-BY-LAYER ANALYSIS\n");
    printf("═══════════════════════\n");
    
    // Layer 2
    struct ethhdr *eth = (struct ethhdr *)packet;
    len_to_skip += ETH_HLEN;
    uint16_t ether_type = ntohs(eth->h_proto);

    printf("ETHERNET II FRAME (Layer 2)\n");
    printf("―――――――――――――――――――――――――――\n");
    printf("Destination MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf("    ∟ Hex: %02X %02X %02X %02X %02X %02X\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf("Source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    printf("    ∟ Hex: %02X %02X %02X %02X %02X %02X\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    
    if(ether_type == ETH_P_IP){
        printf("EtherType: 0x0800 (IPv4)\n");
        ipv4 = 1;
    }
    else if(ether_type == ETH_P_IPV6){
        printf("EtherType: 0x86DD (IPv6)\n");
        ipv6 = 1;
    }
    else if(ether_type == ETH_P_ARP){
        printf("EtherType: 0x0806 (ARP)\n");
        arp = 1;
    }
    printf("    ∟ Hex: %02X %02X\n", (ether_type >> 8) & 0xFF, ether_type & 0xFF);

    printf("\n");

    // Layer 3
    if(ipv4){
        struct iphdr *iph = (struct iphdr *)(packet + len_to_skip);
        len_to_skip += iph->ihl * 4;

        printf("IPv4 HEADER (Layer 3)\n");
        printf("―――――――――――――――――――――\n");
        printf("Version: %u\n", iph->version);
        printf("    ∟ Hex: %02X\n", iph->version);
        printf("Header Length: %d bytes\n", iph->ihl * 4);
        printf("    ∟ Hex: %02X\n", iph->ihl * 4);
        printf("Type of Service: 0x%02X\n", iph->tos);
        printf("    ∟ DSCP: %d, ECN: %d\n", iph->tos >> 2, iph->tos & 3);
        printf("    ∟ Hex: %02X\n", iph->tos);
        printf("Total Length: %d bytes\n", ntohs(iph->tot_len));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(iph->tot_len) >> 8) & 0xFF, ntohs(iph->tot_len) & 0xFF);
        printf("Identification: 0x%04X\n", ntohs(iph->id));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(iph->id) >> 8) & 0xFF, ntohs(iph->id) & 0xFF);
        printf("Flags: 0x%04X\n", ntohs(iph->frag_off));
        printf("    ∟ Reserved: %d, Don't Fragment: %d, More Fragments: %d\n", 
            (ntohs(iph->frag_off) & 0x8000) >> 15, (ntohs(iph->frag_off) & 0x4000) >> 14, (ntohs(iph->frag_off) & 0x2000) >> 13);
        printf("    ∟ Fragment Offset: %d\n", ntohs(iph->frag_off) & 0x1FFF);
        printf("Time to Live: %u\n", iph->ttl);

        char protocol_name[10];
        if(iph->protocol == 6){
            protocol = 6;
            strcpy(protocol_name, "TCP");
        }
        else if(iph->protocol == 17){
            protocol = 17;
            strcpy(protocol_name, "UDP");
        }
        else{
            strcpy(protocol_name, "Unknown");
        }

        printf("Protocol: %u (%s)\n", iph->protocol, protocol_name);
        printf("Header Checksum: 0x%04X\n", ntohs(iph->check));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(iph->check) >> 8) & 0x00FF, ntohs(iph->check) & 0x00FF);
        
        struct in_addr src_addr, dst_addr;
        src_addr.s_addr = iph->saddr;
        dst_addr.s_addr = iph->daddr;

        u_char *src = (u_char *)&iph->saddr;
        u_char *dst = (u_char *)&iph->daddr;

        printf("Source IP: %s\n", inet_ntoa(src_addr));
        printf("    ∟ Hex: %02X %02X %02X %02X\n", src[0], src[1], src[2], src[3]);
        printf("Destination IP: %s\n", inet_ntoa(dst_addr));
        printf("    ∟ Hex: %02X %02X %02X %02X\n", dst[0], dst[1], dst[2], dst[3]);
        printf("\n");
    }

    //##############################-LLM GENERATED CODE STARTS HERE-##############################//
    else if(ipv6){
        struct ip6_hdr *ip6h = (struct ip6_hdr *)(packet + len_to_skip);
        len_to_skip += sizeof(struct ip6_hdr);

        printf("IPv6 HEADER (Layer 3)\n");
        printf("―――――――――――――――――――――\n");
        // Version
        printf("Version: %u\n", (ip6h->ip6_vfc >> 4));
        printf("    ∟ Hex: %02X\n", ip6h->ip6_vfc);

        // Traffic Class & Flow Label
        uint32_t vtf = ntohl(*(uint32_t*)ip6h); // Version, Traffic Class, Flow Label combo
        printf("Traffic Class: %u\n", (vtf & 0x0FF00000) >> 20);
        printf("    ∟ Hex: %02X\n", (vtf & 0x0FF00000) >> 20);
        printf("Flow Label: 0x%05X\n", (vtf & 0x000FFFFF));
        printf("    ∟ Hex: %02X %02X %02X\n", (vtf >> 16) & 0x0F, (vtf >> 8) & 0xFF, vtf & 0xFF);

        // Payload Length
        printf("Payload Length: %u bytes\n", ntohs(ip6h->ip6_plen));
        printf("    ∟ Hex: %02X %02X\n", ((ntohs(ip6h->ip6_plen) >> 8) & 0xFF), ntohs(ip6h->ip6_plen) & 0xFF);

        // Next Header (Protocol)
        printf("Next Header: %u\n", ip6h->ip6_nxt);
        printf("    ∟ Hex: %02X\n", ip6h->ip6_nxt);

        // Hop Limit
        printf("Hop Limit: %u\n", ip6h->ip6_hops);
        printf("    ∟ Hex: %02X\n", ip6h->ip6_hops);

        // Source & Destination Addresses
        char src6[INET6_ADDRSTRLEN], dst6[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &ip6h->ip6_src, src6, sizeof(src6));
        inet_ntop(AF_INET6, &ip6h->ip6_dst, dst6, sizeof(dst6));

        uint8_t *src_bytes = (uint8_t *)&ip6h->ip6_src;
        uint8_t *dst_bytes = (uint8_t *)&ip6h->ip6_dst;
        printf("Source IP: %s\n", src6);
        printf("    ∟ Hex: ");
        for(int i = 0; i < 16; i++) printf("%02X%s", src_bytes[i], i == 15 ? "\n" : " ");
        printf("Destination IP: %s\n", dst6);
        printf("    ∟ Hex: ");
        for(int i = 0; i < 16; i++) printf("%02X%s", dst_bytes[i], i == 15 ? "\n" : " ");
        printf("\n");

        if(ip6h->ip6_nxt == 6){
            protocol = 6;
        }
        else if(ip6h->ip6_nxt == 17){
            protocol = 17;
        }
    }

    else if(arp){
        struct arphdr *arph = (struct arphdr *)(packet + len_to_skip);
        len_to_skip += sizeof(struct arphdr);

        printf("ARP HEADER (Layer 3)\n");
        printf("―――――――――――――――――――――\n");
        printf("Hardware Type: %u\n", ntohs(arph->ar_hrd));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(arph->ar_hrd) >> 8) & 0xFF, ntohs(arph->ar_hrd) & 0xFF);
        printf("Protocol Type: %u\n", ntohs(arph->ar_pro));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(arph->ar_pro) >> 8) & 0xFF, ntohs(arph->ar_pro) & 0xFF);
        printf("Hardware Size: %u\n", arph->ar_hln);
        printf("    ∟ Hex: %02X\n", arph->ar_hln);
        printf("Protocol Size: %u\n", arph->ar_pln);
        printf("    ∟ Hex: %02X\n", arph->ar_pln);
        printf("Opcode: %u\n", ntohs(arph->ar_op));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(arph->ar_op) >> 8) & 0xFF, ntohs(arph->ar_op) & 0xFF);

        // Extract and print sender and target addresses
        // Usually: 6 MAC bytes then 4 IP bytes for both sender and target (for Ethernet/IPv4 ARP)
        uint8_t *data = (uint8_t *)(arph + 1);

        printf("Sender MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            data[0], data[1], data[2], data[3], data[4], data[5]);
        printf("    ∟ Hex: %02X %02X %02X %02X %02X %02X\n",
            data[0], data[1], data[2], data[3], data[4], data[5]);
        printf("Sender IP: %u.%u.%u.%u\n",
            data[6], data[7], data[8], data[9]);
        printf("    ∟ Hex: %02X %02X %02X %02X\n", data[6], data[7], data[8], data[9]);
        printf("Target MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            data[10], data[11], data[12], data[13], data[14], data[15]);
        printf("    ∟ Hex: %02X %02X %02X %02X %02X %02X\n",
            data[10], data[11], data[12], data[13], data[14], data[15]);
        printf("Target IP: %u.%u.%u.%u\n",
            data[16], data[17], data[18], data[19]);
        printf("    ∟ Hex: %02X %02X %02X %02X\n", data[16], data[17], data[18], data[19]);
        printf("\n");
    }

    if(protocol == 6){
        struct tcphdr *tcp = (struct tcphdr *)(packet + len_to_skip);
        len_to_skip += tcp->doff * 4;

        printf("TCP HEADER (Layer 4)\n");
        printf("―――――――――――――――――――――\n");
        printf("Source Port: %u\n", ntohs(tcp->source));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(tcp->source) >> 8) & 0xFF, ntohs(tcp->source) & 0xFF);
        printf("Destination Port: %u\n", ntohs(tcp->dest));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(tcp->dest) >> 8) & 0xFF, ntohs(tcp->dest) & 0xFF);
        printf("Sequence Number: %u\n", ntohl(tcp->seq));
        printf("    ∟ Hex: %02X %02X %02X %02X\n", 
            (ntohl(tcp->seq) >> 24) & 0xFF, (ntohl(tcp->seq) >> 16) & 0xFF,
            (ntohl(tcp->seq) >> 8) & 0xFF, ntohl(tcp->seq) & 0xFF);
        printf("Acknowledgment Number: %u\n", ntohl(tcp->ack_seq));
        printf("    ∟ Hex: %02X %02X %02X %02X\n", 
            (ntohl(tcp->ack_seq) >> 24) & 0xFF, (ntohl(tcp->ack_seq) >> 16) & 0xFF,
            (ntohl(tcp->ack_seq) >> 8) & 0xFF, ntohl(tcp->ack_seq) & 0xFF);
        printf("Header Length: %u bytes\n", tcp->doff * 4);
        printf("    ∟ Hex: %02X\n", tcp->doff << 4);
        printf("Flags: 0x%02X\n", ((uint8_t *)tcp)[13]); // offset to flags byte
        printf("    ∟ URG:%d, ACK:%d, PSH:%d, RST:%d, SYN:%d, FIN:%d\n",
            (tcp->urg?1:0), (tcp->ack?1:0), (tcp->psh?1:0),
            (tcp->rst?1:0), (tcp->syn?1:0), (tcp->fin?1:0));
        printf("    ∟ Hex: %02X\n", ((uint8_t *)tcp)[13]);
        printf("Window Size: %u\n", ntohs(tcp->window));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(tcp->window) >> 8) & 0xFF, ntohs(tcp->window) & 0xFF);
        printf("Checksum: 0x%04X\n", ntohs(tcp->check));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(tcp->check) >> 8) & 0xFF, ntohs(tcp->check) & 0xFF);
        printf("Urgent Pointer: %u\n", ntohs(tcp->urg_ptr));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(tcp->urg_ptr) >> 8) & 0xFF, ntohs(tcp->urg_ptr) & 0xFF);

        // TCP Options (if present)
        int options_len = tcp->doff*4 - sizeof(struct tcphdr);
        if(options_len > 0) {
            printf("TCP Options: %d bytes\n", options_len);
            printf("    ∟ Hex:");
            uint8_t *opts = (uint8_t *)(tcp + 1);
            for(int i = 0; i < options_len; ++i)
                printf(" %02X", opts[i]);
            printf("\n");
        }
        printf("\n");
    }

    if(protocol == 17){
        struct udphdr *udp = (struct udphdr *)(packet + len_to_skip);
        len_to_skip += sizeof(struct udphdr);

        printf("UDP HEADER (Layer 4)\n");
        printf("―――――――――――――――――――――\n");
        printf("Source Port: %u\n", ntohs(udp->source));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(udp->source) >> 8) & 0xFF, ntohs(udp->source) & 0xFF);
        printf("Destination Port: %u\n", ntohs(udp->dest));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(udp->dest) >> 8) & 0xFF, ntohs(udp->dest) & 0xFF);
        printf("Length: %u bytes\n", ntohs(udp->len));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(udp->len) >> 8) & 0xFF, ntohs(udp->len) & 0xFF);
        printf("Checksum: 0x%04X\n", ntohs(udp->check));
        printf("    ∟ Hex: %02X %02X\n", (ntohs(udp->check) >> 8) & 0xFF, ntohs(udp->check) & 0xFF);
        printf("\n");
    }
    //##############################-LLM GENERATED CODE ENDS HERE-##############################//
}