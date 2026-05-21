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

i have no idea what is networking interface's low level stuff
how to config the interface 

let's do a man search on the word "network" in overall man
why network is the better at searching than internet because network is generic term 

How to use man
man categories useful 
2. System Calls "open,read,.."
3. Library calls "printf,"
4. device specific file "null, zero, tty"
7. Miscellaneous 

search `man -k network`
interfaces (5)       - network interface configuration for ifup and ifdown
ifconfig (8)         - configure a network interface
ifdown (8)           - take a network interface down
ifup (8)             - bring a network interface up
ip (8)               - show / manipulate routing, network devices, interfaces and tunnels
ip-link (8)          - network device configuration
ip-netconf (8)       - network configuration monitoring
ip-netns (8)         - process network namespace management
netconfig (5)        - network configuration data base
netdevice (7)        - low-level access to Linux network devices

These are the manuals i filtered from `man -k`
in these i need to create a new interface from c programming 
there is no manual page in category 2,3,4. so take the closest match
so `interfaces (5)`, `ip-netns (8)`, `netdevice (7)` are shortlisted

search each and skim for how to bring new interface 
1. interfaces (5), ip-netns (8) seems like command line config, go next
2. netdevice (7) this promissing, it defines strructure and include files, go head and read for how to bring a device

name - ifreq.ifr_name[IFNAMSIZ]
Ioctls
    SIOCGIFFLAGS - this is getter
        ^
    SIOCSIFFLAGS - this is setter
        ^
    this has to set using ifreq.ifr_flags
        IFF_UP
    --------------
    SIOCSIFNAME -  Changes the name of the interface specified in ifr_name to ifr_newname.  This is a privileged operation.  It is allowed  only  when the interface is not up

 man -k ioctl
ioctl (2)            - control device
 #include <sys/ioctl.h>
 int ioctl(int fd, unsigned long op, ...);
It required file descriptor, so we should open a device

now tap device

man -k tap
devlink-dpipe (8)    - devlink dataplane pipeline visualization
mt (1)               - control magnetic tape drive operation
mt-gnu (1)           - control magnetic tape drive operation
prove (1)            - Run tests through a TAP harness.
rmt (8)              - remote magnetic tape server
rmt-tar (8)          - remote magnetic tape server
slick-greeter-enable-tap-to-click (1) - enable tap-to-click
smbtar (1)           - shell script for backing up SMB/CIFS shares directly to UNIX tape drives
st (4)               - SCSI tape device
tc-taprio (8)        - Time Aware Priority Shaper
twistd3 (1)          - run Twisted applications (TACs, TAPs)

nothing 

$ man -k "virtual device"  
virtual device: nothing appropriate.
$ man -k "virtual network"  
systemd.netdev (5)   - Virtual Network Device configuration
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>

const char* name = "jtap"; 
int main() {
    struct ifreq jtap;
    strcpy(jtap.ifr_name, name);

    // now bring the device up

}