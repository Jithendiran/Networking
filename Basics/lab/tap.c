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

    printf("Executing 'ip link set %s up'\n", dev);
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

    printf("Successfully opened %s.\n", dev_name);

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

/*
Executing 'ip link set tap0 up'
Successfully opened tap0.
Frame: Dest 33:33:00:00:00:16 | Src 42:f4:50:5a:2e:a3 | Type 0x86dd
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:ff:5a:2e:a3 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:16 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:02 | Src 46:f7:6f:38:72:ea | Type 0x86dd
Frame: Dest 33:33:00:00:00:fb | Src 46:f7:6f:38:72:ea | Type 0x86dd

0x86dd -> IPv6
MAC address starting with 33:33 are not physical device. They are IPv6 Multicast MAC addresses.
Now i'm decided not to dig more on mac address 33:33:*

*/

//-----------
/*
What i need?
1. i need to activate bring a new interface for networking
2. make it active
3. receive the traffic in that port and log them
task open a new TAP interface

find TAP related resources 
1. man -k TUP : useless
2. find /usr/include -iname **TUN**
    /usr/include/linux/if_tun.h - InterFace TUNnel
3. grep -r "TUN" /usr/include/linux 
    /usr/include/linux/if_tun.h:#define IFF_TAP             0x0002    
4. device search
    find /dev -iname tun:/dev/net/tun
5. Now find the appropriate data structure
    1. grep -r TUNSETIFF /usr/include/linux/if_tun.h
    #define TUNSETIFF     _IOW('T', 202, int) 
    says int, how would we name inside int, it feels like wrong, this is not the correct structure

    2. check for include chain the file and look for any hint
    grep -r include /usr/include/linux/if_tun.h
    everything here is raw type so no use

    3. see the header file name it is if_tun.h, here tun is the device what is the if means, it is interface
    grep -r -E 'struct if\w*\b' /usr/include/

    now we have `/usr/include/linux/if.h` take a look inside
    ifreq has name 

(alternate way)
5. we know tap is a network device, so search for 
    `man -k network -s 2,3,7` look for (2   System calls, 3   Library calls, 7   Miscellaneous) read the one line description
    short listed
    if_freenameindex (3) - get network interface names and indexes
    if_indextoname (3)   - mappings between network interface names and indexes
    netdevice (7)        - low-level access to Linux network devices
    check each
    if_freenameindex/if_indextoname is only getter 


    `man netdevice` is promissing found the `ifreq`
*/

/*
Task : Open a virtual NIC device with name jitap 
*/
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <string.h>

static const char * name = "jitap"; 
int main(){
    /*
    TUN/TAP is the virtual device, this has to known

    find the related resource
    1.  man -k 'TUP|TAP' | grep -E "network|interface" : useless
    2. find /usr/include -type f \( -iname "*tun*" -o -iname "*tap*" \) or  grep -rE "TUN|TAP" /usr/include/
        Found use full /usr/include/linux/if_tun.h - InterFace TUNnel
    3. device search
         find /dev  -type c \( -iname "tun" -o -iname "tap" \): /dev/net/tun
    */
    int fd = open("/dev/net/tun", O_RDWR);

    /*
    1. man open
    it says error in errno
    so check for 
    2. man errno
    3. err(3), error(3), perror(3), strerror(3)
    */
    if(fd < 0) {
        perror("open TUN");
    }

    /*
    Now need to set the name to config the device need to do the ioctl

    man ioctl

    Now we need the parameter
    1. fd
    2. Flag
        1. grep -ir set /usr/include/linux/if_tun.h
            Know the linux convention 
                1. IF -> InterFace
                2. IFF -> InterFace Flag 
                TUNSETIFF -> TUN -> device SET -> set IFF -> InterFace Flag 
                _IOW means IO Write
            #define TUNSETIFF     _IOW('T', 202, int) 
    3. Data structure
        #define TUNSETIFF     _IOW('T', 202, int) 
            says int, how would  name inside int, it feels like wrong, this is not the correct structure
        
        Method 1. 
            1. check for include chain the file and look for any hint
                grep -r include /usr/include/linux/if_tun.h
                    everything here is raw type so no use
            2. see the header file name it is if_tun.h, here tun is the device what is the if means, it is interface
                grep -r -E 'struct if\w*\b' /usr/include/
                
                now we have `/usr/include/linux/if.h` take a look inside
                ifreq has name 
        Method 2.
            we know tap is a network device, so search for 
        `man -k network -s 2,3,7` look for (2   System calls, 3   Library calls, 7   Miscellaneous) read the one line description
        short listed
        if_freenameindex (3) - get network interface names and indexes
        if_indextoname (3)   - mappings between network interface names and indexes
        netdevice (7)        - low-level access to Linux network devices
        check each
        if_freenameindex/if_indextoname is only getter 


        `man netdevice` is promissing found the `ifreq`
        
    */

    struct ifreq interface;
    memset(&interface, 0, sizeof(struct ifreq));

    strcpy(interface.ifr_name, name);
    
    /*
    man ioctl
    */
    // int res = ioctl(fd, TUNSETIFF, &interface);
    // if(res < 0) {
    //     perror("interface config");

    // }
    // interface config: Invalid argument

    // re-read both /usr/include/linux/if_tun.h and man netdevice
    /*
    if_tun's IFflags only support int, but we passed `struct ifreq`

    in /usr/include/linux/if_tun.h  read the config section for TUNSETIFF, it supports 4 flags TU, TAP, NAPI and NAPI_FRAGS, we need TAP
    */
    // int res = ioctl(fd, TUNSETIFF, IFF_TAP);
    // if(res < 0) {
    //     perror("interface config");

    // }
    // interface config: Bad address

    /*
    read the /usr/include/linux/if.h 's ifreq or man netdevice
    there is a ifru_flags
    */
    interface.ifr_flags = IFF_TAP;
    int res = ioctl(fd, TUNSETIFF, &interface);
    if(res < 0) {
        perror("interface config");

    }
    while (1){}
    return 0;

    /*
    4: jitap: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 32:00:05:9d:7d:4a brd ff:ff:ff:ff:ff:ff
    */
}
