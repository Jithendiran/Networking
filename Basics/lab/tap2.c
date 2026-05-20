// gcc tap2.c -o /tmp/tap_reader
/*
* Author: E.K.Jithendiran
* Date  : 19.5.2026 
*/
#include <stdio.h>              // printf() and perror()
#include <stdlib.h>             // exit()
#include <string.h>             // memset() and strncpy().
#include <unistd.h>             // read() and close()
#include <fcntl.h>              // O_RDWR
#include <linux/if.h>           // ifreq
#include <arpa/inet.h>          // ntohs
#include <sys/ioctl.h>          // ioctl
#include <sys/types.h>          // ssize_t, pid
#include <sys/socket.h>         // SIOCGIFFLAGS
#include <linux/if_tun.h>       // IFF_TAP, TUNSETIFF
#include <netinet/if_ether.h>   // For ETH_P_ALL and ethhdr

inline static const char* eproto_name(uint16_t type){
    switch (type)
    {
        case 0x0800: return "IPv4";
        case 0x0806: return "ARP";
        case 0x86dd: return "IPv6";
        default: return "Unknown";
    }
}

void print_tap_mac(const char *dev) {
    struct ifreq ifr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); return; }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        close(sockfd);
        return;
    }

    unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    close(sockfd);
}

int alloc_tap(char *dev) {
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Read MAC immediately after TUNSETIFF (interface is still DOWN)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    printf("MAC after TUNSETIFF (DOWN) : ");
    print_tap_mac(dev);


    // Get current flags
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("SIOCGIFFLAGS");
        close(sockfd);
        return -1;
    }

    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("SIOCSIFFLAGS (Set Interface Up)");
        close(sockfd);
        return -1;
    }

    // Read MAC after SIOCSIFFLAGS (interface is now UP)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    printf("MAC after SIOCSIFFLAGS (UP): ");
    print_tap_mac(dev);


    close(sockfd);
    return fd;
}

int main() {
    char dev_name[IFNAMSIZ] = "tap0";
    int tap_fd = alloc_tap(dev_name);

    if (tap_fd < 0) return 1;

    printf("Successfully opened %s.\n\n", dev_name);

    unsigned char buffer[2048];
    while (1) {
        ssize_t nread = read(tap_fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("Read error");
            break;
        }

        struct ethhdr *eth = (struct ethhdr *)buffer;
        unsigned short proto = ntohs(eth->h_proto);


        printf("Frame: Dest %02x:%02x:%02x:%02x:%02x:%02x | ",
               eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
               eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

        printf("Src %02x:%02x:%02x:%02x:%02x:%02x | ",
               eth->h_source[0], eth->h_source[1], eth->h_source[2],
               eth->h_source[3], eth->h_source[4], eth->h_source[5]);

        printf("Type %s (0x%04x)\n", eproto_name(proto), proto);
    }

    close(tap_fd);
    return 0;
}
/*
Terminal 1 
$ ip monitor link
5: tap0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN group default 
    link/ether 12:91:c5:b4:5d:6e brd ff:ff:ff:ff:ff:ff
5: tap0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN group default 
    link/ether 12:91:c5:b4:5d:6e brd ff:ff:ff:ff:ff:ff
5: tap0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN group default 
    link/ether 46:f7:6f:38:72:ea brd ff:ff:ff:ff:ff:ff
5: tap0: <BROADCAST,MULTICAST> mtu 1500 qdisc pfifo_fast state DOWN group default 
    link/ether 46:f7:6f:38:72:ea brd ff:ff:ff:ff:ff:ff
Deleted 5: tap0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN group default 
    link/ether 46:f7:6f:38:72:ea brd ff:ff:ff:ff:ff:ff
^C

Terminal 2
$ sudo ./tap_reader 
MAC after TUNSETIFF (DOWN) : 12:91:c5:b4:5d:6e
MAC after SIOCSIFFLAGS (UP): 12:91:c5:b4:5d:6e
Successfully opened tap0.

Frame: Dest 33:33:00:00:00:16 | Src 12:91:c5:b4:5d:6e | Type IPv6 (0x86dd)
Frame: Dest 33:33:ff:b4:5d:6e | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type IPv6 (0x86dd)
^C
*/