/* 
File: getMACAddress.c
Author: mpromonet
Date: 14th October 2019
Purpose: Get the MAC address from the system
Link: https://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program
*/

// #include <stdio.h>
// #include <ifaddrs.h>
// #include <netpacket/packet.h>
// #include <string.h>

// char* getMACAddress ()
// {
//     struct ifaddrs *ifaddr=NULL;
//     struct ifaddrs *ifa = NULL;
//     int i = 0;
//     int j = 0;
    
//     char* macaddress;

//     if (getifaddrs(&ifaddr) == -1)
//     {
//          perror("getifaddrs");
//     }
//     else
//     {
//          for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
//          {
//              if ( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET) )
//              {
//                   struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
                 
                  
//                   if (strcmp(ifa->ifa_name, "eno1") == 0){
//                   //printf("%-8s ", ifa->ifa_name);
//                   for (i=0; i <s->sll_halen; i++){
                   	
// 			// printf("%02x%c", (s->sll_addr[i]), (i+1!=s->sll_halen)?':':'\n');
// 			// macaddress[j] = s->sll_addr[j];
// 			sprintf(&macaddress[j],"%02x%c", s->sll_addr[i], ':');
// 			j+=3;
			
// 			}
                  
//                   }
                  
//              }
//          }
//          freeifaddrs(ifaddr);
//     }
    
    
//     // printf("%s\n", macaddress);
//     return macaddress;
// }

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int getMACAddress(char* macaddress)
{
     char interface[3] = "en0";
     int   mib[6];
     size_t len;
     char            *buf;
     unsigned char       *ptr;
     struct if_msghdr    *ifm;
     struct sockaddr_dl  *sdl;

     //     if (argc != 2) {
     //         fprintf(stderr, "Usage: getmac <interface>\n");
     //         return 1;
     //     }

     mib[0] = CTL_NET;
     mib[1] = AF_ROUTE;
     mib[2] = 0;
     mib[3] = AF_LINK;
     mib[4] = NET_RT_IFLIST;



     if ((mib[5] = if_nametoindex(interface)) == 0) {
          perror("if_nametoindex error");
          exit(2);
     }

     if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
          perror("sysctl 1 error");
          exit(3);
     }

     if ((buf = malloc(len)) == NULL) {
          perror("malloc error");
          exit(4);
     }

     if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
          perror("sysctl 2 error");
          exit(5);
     }

     ifm = (struct if_msghdr *)buf;
     sdl = (struct sockaddr_dl *)(ifm + 1);
     ptr = (unsigned char *)LLADDR(sdl);
     // printf("%02x:%02x:%02x:%02x:%02x:%02x\n", *ptr, *(ptr+1), *(ptr+2),
               // *(ptr+3), *(ptr+4), *(ptr+5));

     sprintf(macaddress, "%02x:%02x:%02x:%02x:%02x:%02x\n", *ptr, *(ptr+1), *(ptr+2),
               *(ptr+3), *(ptr+4), *(ptr+5));
     
     return 1;
}