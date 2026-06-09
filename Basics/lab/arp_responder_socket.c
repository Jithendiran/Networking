// respond arp by using socket
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <unistd.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
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

// read mac packets from socket
static const char * name = "jitap"; 
int main(){

    // name
    unsigned char mac[6];
    unsigned char ip[4];
    int index = -1;
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
    //---------------------------------------------------------------
    int sfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sfd < 0){
        perror("Socket creation");
    }

    
    interface.ifr_flags = interface.ifr_flags | IFF_UP | IFF_MULTICAST;
    res = ioctl(sfd, SIOCSIFFLAGS, &interface);
    if(res < 0) {
        perror("interface up");
        close(fd);
        return 1;
    }

    //-------------------------------------------------------------------
    sleep(5); // assign ip in btw `sudo ip addr add 10.0.0.1/24 dev jitap`

    // get index
    res = ioctl(sfd, SIOCGIFINDEX, &interface);
    if(res < 0) {
        perror("SIOCGIFINDEX");
        close(sfd);
        close(fd);
        return 1;
    }
    index = interface.ifr_ifindex;
    printf("Interface index : %d\n", index);
    
    res = ioctl(sfd, SIOCGIFHWADDR, &interface);
    if(res < 0) {
        perror("SIOCGIFHWADDR");
        close(sfd);
        close(fd);
        return 1;
    }
    memcpy(mac, interface.ifr_hwaddr.sa_data, 6);
    
    close(sfd);

    sfd = socket(AF_INET, SOCK_RAW, 4);
    if(sfd < 0){
        perror("Socket creation");
    }
    
    res = ioctl(sfd, SIOCGIFADDR, &interface);
    if(res < 0) {
        perror("SIOCGIFADDR");
        close(sfd);
        close(fd);
        return 1;
    }
    memcpy(ip, interface.ifr_addr.sa_data+2, 4);
    close(sfd);
    //----------------------------------------------------------------------
    printf("Mac address : %02x:%02x:%02x:%02x:%02x:%02x\n", 
        (unsigned char)mac[0], (unsigned char)mac[1], (unsigned char)mac[2],
        (unsigned char)mac[3], (unsigned char)mac[4], (unsigned char)mac[5]);
    printf("Ip address %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
    char buf[1500];

    //--------------------------------------------------------------------------------------------
    // now create a socket to respond arp
    // man 7 socket -> give information to check man 2 socket
    // from man 2 socket choose AF_PACKET because of word low level, hint man 7 packet
    // SOCK_RAW choose from man 7 packet
    // man 7 packet gave iformation to check for protocol in if_ether page, i need arp
    // man page says "The link-level header information is available in a common format in a sockaddr_ll structure"
    sfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sfd < 0){
        perror("Socket creation");
    }
    //  man 7 packet says "get packets only from a specific  interface  use bind(2)"

    // grep -rn "struct sockaddr_ll {" /usr/include/ -> /usr/include/linux/if_packet.h:14:struct sockaddr_ll {

    struct sockaddr_ll device;
    memset(&device, 0, sizeof(device));
    device.sll_family = AF_PACKET;
    device.sll_protocol = htons(ETH_P_ALL);
    device.sll_pkttype = PACKET_BROADCAST;
    device.sll_halen = 6;
    device.sll_ifindex = index;
    memcpy(device.sll_addr, mac, 6);
    
    // man 2 bind, packet man says bind accept sockaddr_ll, but bind says it accept sockadd which one to use?
    // see man 2 bind example
    if(bind(sfd, (struct sockaddr *)&device, sizeof(device)) < 0) {
        perror("Bind");
    }
    printf("start reading\n");
    while (1){
        res = read(sfd, buf, 1500);
        if(res < 0){
            perror("Read socket");
            return 1;
        }
        
        // receiver mac from sockaddr_ll
        printf("data");
        fflush(stdout);
        
    }
    close(sfd);
    close(fd);
    return 0;

}

// debug why data not printing when  arping 10.0.0.1 -I jitap