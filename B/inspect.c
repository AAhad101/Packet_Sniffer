#include "inspect.h"
/*
void inspect_packet(int analysed_num){
    int index = analysed_num - 1;
    PacketRecord cur_packet = sniff_log[index];

    u_char *packet = cur_packet.packet;
    int caplen = cur_packet.captured_length;
    int frame_len = cur_packet.frame_length;
    
    int len_to_skip = 0;
    int ipv4 = 0;
    int ipv6 = 0;
    int arp = 0;

    printf("################################################################################\n");
    printf("#                       C-SHARK DETAILED PACKET ANALYSIS                       #\n");
    printf("################################################################################\n");

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
    printf("═══════════════════════");

    for(int i = 0; i < caplen; i++){
        printf("%02X ", packet[i]);
        if((i+1) % 16 == 0){        // Adjacently print ASCII interpretation
            for(int j = i-15; j <= i; j++){
                unsigned char c = packet[j];
                if(c >= 32 && c <= 126) printf("%c", c);
                else printf(".");
            }
            printf("\n");
        }
    }
    if(caplen % 16 != 0){       // Padding the last row accordingly
        for(int i = 0; i < 16 - (caplen % 16); i++){
            printf("   ");
        }       
        
        int div = caplen / 16;
        for(int i = 0; i < caplen % 16; i++){
            unsigned char c = packet[div * 16 + i];
            if(c >= 32 && c <= 126) printf("%c", c);
            else printf(".");
        }
        printf("\n");
    }

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

    // Layer 3
    if(ipv4){
        struct iphdr *iph = (struct iphdr *)

        printf("IPV4 HEADER (Layer 3)\n");
        printf("―――――――――――――――――――――\n");
        printf("Version: %u\n", ntohs(iph->version));
        printf("    ∟ Hex: %02X\n", ntohs(iph->version));
        printf("Header Length: %d bytes\n", iph->ihl * 4);
        printf("    ∟ Hex: %02X\n", iph->ihl * 4);
        printf("Type of Service: 0x%02X\n", ntohs(iph->tos));
        printf("    ∟ DSCP: %d, ECN: %d\n", ntohs(iph->tos) >> 2, ntohs(iph->tos) & 3);
        printf("    ∟ Hex: %02X\n", ntohs(iph->tos));
        printf("Total Length: %d bytes\n", ntohs(iph->tot_len));
        printf("    ∟ Hex: %02X %02X\n", ntohs(iph->tot_len) >> 2, ntohs(iph->tot_len) & 3);
        printf("Identification: 0x%04X\n", ntohs(iph->id));
        printf("    ∟ Hex: %02X %02X\n", ntohs(iph->id) >> 2, ntohs(iph->id) & 3);
        printf("Flags: 0x%04X\n", ntohs(iph->frag_off));
        printf("    ∟ Reserved: %d, Don't Fragment: %d, More Fragments: %d\n", 
            (ntohs(iph->frag_off) & 0x8000) >> 15, (ntohs(iph->frag_off) & 0x4000) >> 14, (ntohs(iph->frag_off) & 0x2000) >> 13);
        printf("    ∟ Fragment Offset: %d\n", ntohs(iph->frag_off) & 0x1FFF);
    }
}*/