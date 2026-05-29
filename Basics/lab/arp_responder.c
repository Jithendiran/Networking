// gcc arp_responder.c -o /tmp/arp_responder
/*
* Author: E.K.Jithendiran
* Date  : 27.5.2026 
*/
/*
1. Open and up interface
2. From different terminal set the manual IP address `sudo ip addr add 10.0.0.1/24 dev jitap`
3. Listen to the incoming traffic on this interface
4. If a ARP resquest is received for it's own IP respond it with it's mac address
--------------
5. Get it's MAC address
6. Refer: https://datatracker.ietf.org/doc/html/rfc826
7. grep -rn "arp" /usr/include 
    /usr/include/net/if_arp.h is related
*/

/*
Packet format (rfc826)
1. 48bit: Ethernet address of destination
2. 48bit: Ethernet address of sender
3. 16bit: Protocol type = ether_type$ADDRESS_RESOLUTION
-------------
Data
1. 16bit: (ar$hrd)Hardware address space (e.g., Ethernet, Packet Radio Net.) = Ethernet
2. 16bit: (ar$pro)Protocol address space.  For Ethernet  hardware, this is from the set of type fields =
3.  8bit: (ar$hln)byte length of each hardware address = 6
4.  8bit: (ar$pln)byte length of each protocol address = length of ar$pro
5. 16bit: (ar$op)opcode (ares_op$REQUEST | ares_op$REPLY) = ares_op$REQUEST
6. nbyts: (ar$sha)Hardware address of sender of this packet, n from the ar$hln field. = 48bit requester address
7. mbyts: (ar$spa)Protocol address of sender of this packet, m from the ar$pln field. = protocol address of itself
9. nbyts: (ar$tha)Hardware address of target of this packet. = 48bit ---- not set
10.mbyts: (ar$tpa)Protocol address of target. = protocol address of the machine that is trying to be accessed
*/
/*
SAMPLE RUN (tap.c uncommented raw print)
$ arping 10.0.0.1 -I jitap
from jitap , reaching it's own IP so the IP address of is same for src and dst protocol address

Src MAC: 32:00:05:9d:7d:4a      Dst MAC: ff:ff:ff:ff:ff:ff      Protocol: ARP(0x0806)
Size : 42       0xff 0xff 0xff 0xff 0xff 0xff  0x32 0x00 0x05 0x9d 0x7d 0x4a 0x08 0x06 0x00 0x01 0x08 0x00 0x06 0x04 0x00 0x01 0x32 0x00 0x05 0x9d 0x7d 0x4a 0x0a 0x00 0x00 0x01 0xff 0xff 0xff 0xff 0xff 0xff 0x0a 0x00 0x00 0x01 

 Header

 dst: 0xff 0xff 0xff 0xff 0xff 0xff 
 src: 0x32 0x00 0x05 0x9d 0x7d 0x4a
 0x08 0x06
 ---
 0x00 0x01 = 0001
 0x08 0x00 = 0800
 0x06
 0x04
 0x00 0x01 = 0001
 0x32 0x00 0x05 0x9d 0x7d 0x4a
 0x0a 0x00 0x00 0x01 = 10.0.0.1
 0xff 0xff 0xff 0xff 0xff 0xff
 0x0a 0x00 0x00 0x01 = 10.0.0.1
*/
/*
Tiny working
if header's 0x08 0x06 
--------
1. Received Ethernet frame, ar$hrd == ARPHRD_ETHER : no matche ignore the packet
        To check the available types read /usr/include/net/if_arp.h :  ARP protocol HARDWARE identifiers
2. Received protocol address, ar$pro == 0x800 : no matche ignore the packet
        /usr/include/net/if_arp.h has no ref, but we have seen 0x0800 in /usr/include/linux/if_ether.h :  Ethernet Protocol ID's.
3. Received ar$hln == 6 for mac address length : no matche ignore the packet
4. Received ar$pln == 4 for protocol address length : no matche ignore the packet
5. Received ar$op  == 1 for ARP protocol opcodes: 
6. Received ar$sha not null
7. Received ar$spa not null
8. Received ar$tha == broadcast
9. Received ar$tpa == it's IP
---
construct reply
    Header
    1. DST address from ARP request's ar$sha
    2. SRC address from it's own
    3. protocol is ARP (0x080x06)
    payload
    1. ETHERNET 0x0001
    2. IPV4 0x0800
    3. ar$hln = 6
    4. ar$pln = 4
    5. ar$op  = 2
    6. ar$sha = it's own mac
    7. ar$spa = It's IP
    8. ar$tha = ARP request's ar$sha
    9. ar$tpa = ARP request's ar$spa

write to the socket
        
*/
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

static const char * protoname(u_int16_t type){
    switch (type)
    {
        case 0x0800:    return "IPv4";
        case 0x0806:    return "ARP";
        case 0x86DD:    return "IPv6";
        default:        return "Unknown";
    }
}

static const char * name = "jitap"; 
int main(){
    int fd = open("/dev/net/tun", O_RDWR);

    if(fd < 0) {
        perror("open TUN");
        return 1;
    }


    struct ifreq interface;
    memset(&interface, 0, sizeof(struct ifreq));

    strcpy(interface.ifr_name, name);
    interface.ifr_flags = IFF_TAP | IFF_NO_PI;

    int res = ioctl(fd, TUNSETIFF, &interface);
    if(res < 0) {
        perror("interface config");

    }
    
    int sfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sfd < 0){
        perror("Socket creation");
    }

    // now try from the start
    interface.ifr_flags = interface.ifr_flags | IFF_UP | IFF_MULTICAST;
    res = ioctl(sfd, SIOCSIFFLAGS, &interface);
    if(res < 0) {
        perror("interface up");
        close(fd);
        return 1;
    }


    // read MAC address, to up the interface we use socket, so mostly using only socket we can find it's MAC address
    //1. $ man  socket // no use
    //2. $  man -k address
    
    /*
    //2. $  man -k address 
ether_aton (3)       - Ethernet address manipulation routines
ether_aton_r (3)     - Ethernet address manipulation routines
ether_hostton (3)    - Ethernet address manipulation routines
ether_line (3)       - Ethernet address manipulation routines
ether_ntoa (3)       - Ethernet address manipulation routines
ether_ntoa_r (3)     - Ethernet address manipulation routines
freehostent (3)      - get network hostnames and addresses
freeifaddrs (3)      - get interface addresses
sockaddr (3type)     - socket address
sockaddr_in (3type)  - socket address

------------------
freehostent (3)      - get network hostnames and addresses
1. man 3 freehostent
// Use getaddrinfo(3) and getnameinfo(3) instead.
2. man 3 getaddrinfo
// struct sockaddr  // search for this word
// reference inside net/if.h, either read `man 7 netdevice` or `grep -r "IFF_UP" /usr/include`
/usr/include/linux/if.h here need to find the ioctl flag to get the infomation from interface
search for grep -rn SIOCSIFFLAGS /usr/include
found ref /usr/include/x86_64-linux-gnu/bits/ioctls.h look for hardware address related 
SIOCGIFHWADDR
    */

    res = ioctl(sfd, SIOCGIFHWADDR, &interface);
    if(res < 0) {
        perror("SIOCGIFHWADDR");
        close(sfd);
        close(fd);
        return 1;
    }
// interface.ifr_hwaddr
    printf("Address  %02x:%02x:%02x:%02x:%02x:%02x\n", (unsigned char)interface.ifr_hwaddr.sa_data[0], (unsigned char)interface.ifr_hwaddr.sa_data[1],
    (unsigned char)interface.ifr_hwaddr.sa_data[2],(unsigned char)interface.ifr_hwaddr.sa_data[3], (unsigned char)interface.ifr_hwaddr.sa_data[4], (unsigned char)interface.ifr_hwaddr.sa_data[5]);
    // matching with the address but need a way to read it through library call
    // socket address to ether address

    // read ip address


    // no longer needed
    close(sfd);
    
    char buf[1500];
    
    while (1){
        res = read(fd, buf, 1500);
        if(res < 0){
            perror("Read socket");
            return 1;
        }
        
        struct ethhdr *frame = (struct ethhdr *)buf;
        int proto = ntohs(frame->h_proto);
        printf("Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\t",
            frame->h_source[0],frame->h_source[1],frame->h_source[2],
            frame->h_source[3],frame->h_source[4],frame->h_source[5]);
        printf("Dst MAC: %02x:%02x:%02x:%02x:%02x:%02x\t",
            frame->h_dest[0],frame->h_dest[1],frame->h_dest[2],
            frame->h_dest[3],frame->h_dest[4],frame->h_dest[5]);

        printf("Protocol: %s(0x%04x)\n", protoname(proto), proto);
    }
    close(fd);
    return 0;

}