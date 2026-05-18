// gcc tap.c -o /tmp/tap_reader
/*
* Author: E.K.Jithendiran
* Date  : 18.5.2026 
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

int alloc_tap(char *dev) {
    struct ifreq ifr;
    int fd, err;

    // Open the clone device
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    // IFF_TAP: Ethernet-level frame (Layer 2)
    // IFF_NO_PI: Don't provide packet information (keeps it raw)
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // Create the device
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);

    // ip link up
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&ifr.ifr_flags, 0, sizeof(ifr.ifr_flags));
    
    // Get current flags
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
    close(sockfd);

    return fd;
}

int main() {
    char dev_name[IFNAMSIZ] = "tap0";
    int tap_fd = alloc_tap(dev_name);

    if (tap_fd < 0) return 1;

    printf("Successfully opened %s. Run 'ip link set %s up' to start.\n", dev_name, dev_name);

    unsigned char buffer[2048];
    while (1) {
        /*
        If no packets arrived, program will be paused, it won't move to next line of code
        CPU won't execute till packet arrives
        */
        ssize_t nread = read(tap_fd, buffer, sizeof(buffer));
        // +ve bytes received, 0 End Of File, -1 error occured
        if (nread < 0) {
            perror("Read error");
            break;
        }

        // The first part of the buffer is the Ethernet header
        struct ethhdr *eth = (struct ethhdr *)buffer;

        printf("Frame: Dest %02x:%02x:%02x:%02x:%02x:%02x | ",
               eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
               eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

        printf("Src %02x:%02x:%02x:%02x:%02x:%02x | ",
               eth->h_source[0], eth->h_source[1], eth->h_source[2],
               eth->h_source[3], eth->h_source[4], eth->h_source[5]);

        // Protocol type (e.g., 0x0800 for IPv4)
        printf("Type 0x%04x\n", ntohs(eth->h_proto));
    }

    close(tap_fd);
    return 0;
}