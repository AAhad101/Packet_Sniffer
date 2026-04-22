#include "general.h"
#include "signals.h"
#include "pkt_handler.h"
#include "inspect.h"

pcap_t *handle = NULL;     // Made handle a global variable for handling Ctrl-C signal interrupt
int pkt_num = 0;
int selected_proto = 0;
int session_occur = 0;
int ctrlc = 0;

PacketRecord sniff_log[MAX_PACKETS];    // Stored the packets from the latest sniffing session

int main(){
    signal(SIGINT, sigint_handler);
    // Printing initial text
    printf("\n");
    printf("[C-Shark] The Command-Line Packet Predator\n");
    printf("==============================================\n");

    printf("[C-Shark] Searching for available interfaces... ");

    // Fetching and printing all available network interfaces
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];
    if(pcap_findalldevs(&alldevs, errbuf) == -1){      // Error in pcap_findalldevs()
        fprintf(stderr, "pcap_findalldevs() failed.\n\n");
        exit(1);
    }

    // If alldevs is NULL, the head of the linked list is NULL i.e. it is empty
    if(alldevs == NULL){
        printf("No available network interfaces.\n");
    }

    else{
        printf("Found!\n\n");
        pcap_if_t *head = alldevs;
        int i = 0;

        // Traversing through linked list to print all available network interfaces
        while(head){
            i++;
            printf("%d. %s", i, head->name);
            if(head->description) printf(" (%s)", head->description);
            printf("\n");
            head = head->next;
        }
        printf("\n");

        // Giving option to the user to select the interface to sniff
        int net_int = 0;
        printf("Select an interface to sniff (1-%d): ", i);
        int ret = scanf(" %d", &net_int);

        if(ret == EOF){   // Exit C-Shark when user presses Ctrl-D
            printf("^D\n");
            return 0;
        }

        // Error message if the user enters an invalid number
        if(net_int < 1 || net_int > i){
            fprintf(stderr, "Invalid network interface number selected.\n\n");
            exit(1);
        }

        // Finding the interface selected from the linked list
        pcap_if_t *selected_int = find_interface(alldevs, net_int);
        printf("\n");
        printf("[C-Shark] Interface '%s' selected. What's next?\n\n", selected_int->name);
        
        while(1){
            ctrlc = 0;

            printf("1. Start Sniffing (All Packets)\n");
            printf("2. Start Sniffing (With Filters)\n");
            printf("3. Inspect Last Session\n");
            printf("4. Exit C-Shark\n\n");
            printf("Enter your option: ");

            // Taking user's input
            int option = 0;
            int val = scanf("%d", &option);
            if(val == EOF){
                printf("^D\n");
                return 0;
            }
            printf("\n");

            if(option == 1){
                pkt_num = 0;

                handle = pcap_open_live(selected_int->name, 65536, 1, 1, errbuf);    // Handle of device to be sniffed
                if(handle == NULL){
                    fprintf(stderr, "Couldn't open device for sniffing.\n\n");
                    continue;
                }

                if(!session_occur) session_occur = 1;

                while(ctrlc == 0){
                    fd_set rfds;
                    FD_ZERO(&rfds);
                    FD_SET(STDIN_FILENO, &rfds);

                    struct timeval tv;
                    tv.tv_sec = 0;
                    tv.tv_usec = 10;

                    int select_ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
                    if(select_ret == -1){
                        fprintf(stderr, "Error: select() failure.\n\n");
                        break;
                    }

                    if(select_ret == 0){
                        pcap_dispatch(handle, -1, packet_handler, NULL);
                    }

                    if(FD_ISSET(STDIN_FILENO, &rfds)){
                        char ctrld[1024];
                        if(fgets(ctrld, sizeof(ctrld), stdin) == NULL){
                            printf("^D\n");
                            return 0;
                        }
                    }
                }
            }

            else if(option == 2){
                pkt_num = 0;

                printf("Select from the following protocols to filter:\n");
                printf("1. HTTP\n");
                printf("2. HTTPS\n");
                printf("3. DNS\n");
                printf("4. ARP\n");
                printf("5. TCP\n");
                printf("6. UDP\n");

                selected_proto = 0;
                printf("Enter your option: ");
                int ded = scanf("%d", &selected_proto);
                if(ded == EOF){
                    printf("^D\n");
                    return 0;
                }

                if(selected_proto < 1 || selected_proto > 6){
                    fprintf(stderr, "Invalid option selected.\n\n");
                    continue;
                }

                handle = pcap_open_live(selected_int->name, 65536, 1, 1, errbuf);  // Handle of device to be sniffed
                if(handle == NULL){
                    fprintf(stderr, "Couldn't open device for sniffing.\n\n");
                    continue;
                }

                // If you want to do display_filter
                //pcap_loop(handle, -1, display_filter, NULL); 

                // Capture filter
                char filter_exp[30];
                if(selected_proto == 1){         // HTTP
                    strcpy(filter_exp, "tcp port 80 or udp port 80");
                }
                else if(selected_proto == 2){    // HTTPS
                    strcpy(filter_exp, "tcp port 443 or udp port 443");
                }
                else if(selected_proto == 3){    // DNS
                    strcpy(filter_exp, "tcp port 53 or udp port 53");
                }
                else if(selected_proto == 4){    // ARP
                    strcpy(filter_exp, "arp");
                }
                else if(selected_proto == 5){    // TCP
                    strcpy(filter_exp, "tcp");
                }
                else if(selected_proto == 6){    // UDP
                    strcpy(filter_exp, "udp");
                }

                struct bpf_program fp;
                if(pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1){
                    fprintf(stderr, "Couldn't parse capture filter\n\n");
                    pcap_close(handle);
                    continue;
                }

                if(pcap_setfilter(handle, &fp) == -1){
                    fprintf(stderr, "Couldn't install capture filter\n\n");
                    pcap_freecode(&fp);
                    pcap_close(handle);
                    continue;
                }

                pcap_freecode(&fp);

                if(!session_occur) session_occur = 1; 

                while(ctrlc == 0){
                    // Handling Ctrl-D using select
                    fd_set rfds;
                    FD_ZERO(&rfds);
                    FD_SET(STDIN_FILENO, &rfds);

                    struct timeval tv;
                    tv.tv_sec = 0;
                    tv.tv_usec = 10;

                    int select_ret = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
                    if(select_ret == -1){
                        fprintf(stderr, "Error: select() failure.\n\n");
                        break;
                    } 
                    
                    if(select_ret == 0){
                        pcap_dispatch(handle, -1, packet_handler, NULL);
                    }

                    if(FD_ISSET(STDIN_FILENO, &rfds)){
                        char ctrld[1024];
                        if(fgets(ctrld, sizeof(ctrld), stdin) == NULL){
                            printf("^D\n");
                            return 0;
                        }
                    }
                }
            }

            else if(option == 3){
                if(!session_occur){
                    printf("There hasn't been any sniffing session yet.\n\n");
                    continue;
                }

                print_session();
                printf("\n");

                int analyse_num = 0;

                while(1){
                    printf("Enter the number of the packet to inspect: ");
                    int inp = scanf("%d", &analyse_num);
                    if(inp == EOF){
                        printf("^D\n");
                        return 0;
                    }
                    printf("\n");
                    
                    if(analyse_num < 1 || analyse_num > pkt_num){
                        fprintf(stderr, "Invalid packet number entered.\n\n");
                        continue;
                    }

                    inspect_packet(analyse_num);
                }
            }

            // If option 4 is selected, break out of the loop to exit
            else if(option == 4){
                printf("Exiting C-Shark.\n");
                break;
            }

            // Error message if the user selects invalid option
            else{
                /*int c;
                while ((c = getchar()) != '\n' && c != EOF);    // To clear scanf buffer*/
                fprintf(stderr, "Invalid option selected.\n\n");
                continue;
            }
        
        }

        pcap_freealldevs(alldevs);
    }

    return 0;
}
