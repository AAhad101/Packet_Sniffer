#include "general.h"

pcap_if_t *find_interface(pcap_if_t *alldevs, int index){
    int i = 1;
    pcap_if_t *head = alldevs;
    while(head && i != index){
        head = head->next;
        i++;
    }
    return head;
}