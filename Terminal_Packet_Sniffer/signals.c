#include "signals.h"
#include "general.h"

void sigint_handler(){
    if(handle != NULL){
        printf("\n\n");
        pcap_breakloop(handle);
        handle = NULL;
        ctrlc = 1;
    }
}