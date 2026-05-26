// gcc tap.c -o /tmp/tap_reader
/*
* Author: E.K.Jithendiran
* Date  : 18.5.2026 
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
    /*
    TUN/TAP is the virtual device, this has to known

    find the related resource
    1.  man -k 'TUP|TAP' | grep -E "network|interface" : useless
    2. `find /usr/include -type f \( -iname "*tun*" -o -iname "*tap*" \)` 
            Find any file name has `tun` or `tap` 
        or  
        `grep -rE "TUN|TAP" /usr/include/`
            search the word TUN or TAP in the files inside folder /usr/include/

        Found use full /usr/include/linux/if_tun.h - InterFace TUNnel
    3. device search
         find /dev  -type c \( -iname "tun" -o -iname "tap" \): /dev/net/tun
    */
    int fd = open("/dev/net/tun", O_RDWR);

    /*
    1. man open  -> says errno is set on failure
    2. man errno  -> need to print errno 
    3. err(3), error(3), perror(3), strerror(3)
    */
    if(fd < 0) {
        perror("open TUN");
    }

    /*
    Goal: configure the TAP interface name via ioctl.

    `man ioctl`

    Now we need the parameter
    1. file descriptor: fd
    2. op: Discovering the Request Code (op)
        1. grep -ir set /usr/include/linux/if_tun.h
            Know the linux convention 
                1. IF -> InterFace
                2. IFF -> InterFace Flag 
                TUNSETIFF -> TUN -> device SET -> set IFF -> InterFace Flag 
                _IOW means IO Write
             Target Definition found for grep:
                `#define TUNSETIFF     _IOW('T', 202, int) `
    3. Data structure
        #define TUNSETIFF     _IOW('T', 202, int) 
            The macro definition references an 'int', but we need to pass a string (the interface name "jitap"). An 'int' cannot hold a string.
        
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
            1. if_freenameindex (3) - get network interface names and indexes
            2. if_indextoname (3)   - mappings between network interface names and indexes
            3. netdevice (7)        - low-level access to Linux network devices
            
            check each
            if_freenameindex/if_indextoname is only getter 
            `man netdevice` is promissing found the `ifreq`
        Method 3. 
            Search in kernel docs https://docs.kernel.org/networking/tuntap.html
        
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

    //----------------------------------

    // re-read both /usr/include/linux/if_tun.h and `man netdevice
    /*
    if_tun's IFflags only support int, but we passed `struct ifreq`

    in /usr/include/linux/if_tun.h  read the config section for TUNSETIFF, it supports 4 flags TU, TAP, NAPI and NAPI_FRAGS, we need TAP
    */

    // int res = ioctl(fd, TUNSETIFF, IFF_TAP);
    // if(res < 0) {
    //     perror("interface config");

    // }
    // interface config: Bad address // ioctl expect address

    //------------------------------------

    /*
    read the /usr/include/linux/if.h 's ifreq or man netdevice
    there is a ifru_flags
    */
    interface.ifr_flags = IFF_TAP | IFF_NO_PI; // 1st time remove `IFF_NO_PI` and run, then read the comment inside loop
    int res = ioctl(fd, TUNSETIFF, &interface);
    if(res < 0) {
        perror("interface config");

    }
    /*
    4: jitap: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 32:00:05:9d:7d:4a brd ff:ff:ff:ff:ff:ff
    */

    /*
    Goal enable the device
    Now we should follow the ifreq related ioctl op codes, not if_tun's IOCTL opcode
    because now we are configure network interface, not a virtual device, ifreq is commoon

    1. man netdevice
    SIOCSIFFLAGS -> 
    IFF_UP            Interface is running.
    */

    // interface.ifr_flags = interface.ifr_flags | IFF_UP;

    // res = ioctl(fd, SIOCSIFFLAGS, &interface);
    // if(res < 0) {
    //     perror("interface up");

    // }

    // interface up: Invalid argument

    // error reason might be SIOCSIFFLAGS don't know about IFF_TAP, so create new flag

    // struct ifreq config;
    // memset(&config, 0, sizeof(struct ifreq));
    // config.ifr_flags =  IFF_UP;

    // res = ioctl(fd, SIOCSIFFLAGS, &config);
    // if(res < 0) {
    //     perror("config up");
    // }

    // still error

    /*
    serach for IOCTL's param
    1. check in flag symbol
        grep -r SIOCSIFFLAGS /usr/include
            $ /usr/include/x86_64-linux-gnu/bits/ioctls.h:#define SIOCSIFFLAGS        0x8914
            no luck
            how to find it's param type
    */
   // issue is as per `man netdevice` it require socket's file descriptor

        /*
    man -k socket -s 2,3,7
    socket (7)           - Linux socket interface

    socket is an endpoint for communication

        It has 3 arguments 
        1. domain
        2. type
        3. protocol

        Domain : Defines addressing nature
            * AF_INET - ipv4
            * AF_INET6 - IPv6
            * AF_PACKET - Low-level packet interface

            `man 7 address_families`

        Type: Nature of communication 
            * SOCK_STREAM - sequenced, reliable, two-way, connection-based byte streams.
            * SOCK_DGRAM - connectionless, unreliable messages
            * SOCK_RAW - Provides raw network protocol access. 
                - SOCK_DGRAM and SOCK_RAW sockets allow sending of datagrams
            * SOCK_PACKET  - is an obsolete socket type to receive raw packets directly from the device driver
                - This is the RAW packet see `man 7 packet`.

        Protocol: 
            Usually for the type of socket single protocol will be present, in this case it can be specified as 0
            example SOCK_STREAM: TCP, SOCK_DGRAM: UDP
            How ever It is possible that multiple protocol may exists for a type
            example: SOCK_PACKET: TCP, UDP, ICMP

            `man 5 protocols`

    man 7 packet
        - SOCK_RAW for raw packets including the link-level header
        - SOCK_DGRAM for cooked packets with the link-level  header  removed
        - When protocol is set to htons(ETH_P_ALL), then all protocols are received. All incoming packets of that protocol type will be passed to the packet socket before they are passed to the protocols implemented in the kernel
        - If  protocol is set to zero, no packets are received.
        - SOCK_RAW is similar to but not compatible with the obsolete AF_INET/SOCK_PACKET of Linux 2.0

    Notes
        From here got to know 
        * Domain: AF_PACKET
        * Type: SOCK_RAW or SOCK_PACKET
            * need to check the difference
        * Protocol: htons(ETH_P_ALL) : is this only for SOCK_RAW?
        
        Which has to use SOCK_RAW or SOCK_PACKET? seems like both for same purpose
        let's use AF_PACKET, SOCK_RAW and ETH_P_ALL
        
    */
    int sfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sfd < 0){
        perror("Socket creation");
    }

    // now try from the start
    interface.ifr_flags = interface.ifr_flags | IFF_UP;
    res = ioctl(sfd, SIOCSIFFLAGS, &interface);
    if(res < 0) {
        perror("interface up");
    }

    // up successfully, socket identify happens by the name 
    /*
    5: jitap: <BROADCAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN mode DEFAULT group default qlen 1000
    link/ether 32:00:05:9d:7d:4a brd ff:ff:ff:ff:ff:ff
    */

    /*
    One problem in previous output Multicast is there but it is removed
    read the `man netdevices` check the flag section and add `IFF_MULTICAST`
    */

    // now try from the start
    interface.ifr_flags = interface.ifr_flags | IFF_MULTICAST;
    res = ioctl(sfd, SIOCSIFFLAGS, &interface);
    if(res < 0) {
        perror("interface up");
    }
    /*
    6: jitap: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN mode DEFAULT group default qlen 1000
    link/ether 32:00:05:9d:7d:4a brd ff:ff:ff:ff:ff:ff
    */

   /*
    * Now goal is to log the receiving packets from the interface
    * we have 2 fd's one for virtual interface and one for socket
    * If we use hardware interface we would directly use the socket step by name, so the read only happens through socket
    * also as per `man socket` read(2), recv(2) used for reading
    */
    char buf[1500];
    
    while (1){
        /* 
        man 2 read
        1. return 0 -> EOF
        2. return n <= requested -> available to read
        3. return -1 error, set tp errno 
        */
        // res = read(sfd, buf, 1500); // issue with socket descriptor, it is reading packet from all the available interface
                                        // but need only jitap, so using fd for reading
        res = read(fd, buf, 1500);
        if(res < 0){
            perror("Read socket");
            return 1;
        }

        // in this 1024 will it contain more than 1 ethernet frame? how to identify that?
        /*
        * Stream sockets (SOCK_STREAM) — read() returns however many bytes are available, may span multiple messages.
        * Datagram sockets (SOCK_RAW, SOCK_DGRAM) — read() returns exactly one datagram per call.
        */
        
        /*
        Now what is the data structure to read? 
        SOCK_RAW which includes layer 2 packet, it is ethernet
        search for 
        1. `man -k eth`
        2. grep -r eth /usr/include (many file)
        3. from the above things we know if_ so search for this pattern any luck
            find /usr/include -name "if_*.h" -exec grep -H "eth" {} +
            short list
            1. /usr/include/linux/if_link.h
            2. /usr/include/linux/if_ether.h
                -> has ethhdr
        */


        
        struct ethhdr *frame = (struct ethhdr *)buf;
        int proto = ntohs(frame->h_proto);
        printf("Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\t",
            frame->h_source[0],frame->h_source[1],frame->h_source[2],
            frame->h_source[3],frame->h_source[4],frame->h_source[5]);
        printf("Dst MAC: %02x:%02x:%02x:%02x:%02x:%02x\t",
            frame->h_dest[0],frame->h_dest[1],frame->h_dest[2],
            frame->h_dest[3],frame->h_dest[4],frame->h_dest[5]);

        // to convert protol to String refer /usr/include/linux/if_ether.h
        printf("Protocol: %s(0x%04x)\n", protoname(proto), proto);

        // run without `IFF_NO_PI` flag
        /*
        Src MAC: 00:00:00:16:b2:6a      Dst MAC: 00:00:86:dd:33:33      Protocol: Unknown(0x1b37)
        Src MAC: 00:00:00:16:32:00      Dst MAC: 00:00:86:dd:33:33      Protocol: Unknown(0x059d)
        Src MAC: ff:37:f7:ea:32:00      Dst MAC: 00:00:86:dd:33:33      Protocol: Unknown(0x059d)
        */

        // in /usr/include/linux/if_ether.h there is no such protocols, and src/dst mac address is wrong
        
        // print the hex dump
        // printf("Size : %d\t",res);
        // for(int i = 0; i < res; i++){
        //     printf("0x%02x ",(unsigned char)buf[i]);
        // }
        // printf("\n");

        /*
            Size : 94       0x00 0x00 0x86 0xdd 0x33 0x33 0x00 0x00 0x00 0x16 0x0e 0x3d 0xe2 0x24 0x37 0xe8 0x86 0xdd 0x60 0x00 0x00 0x00 0x00 0x24 0x00 0x01 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0xff 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x16 0x3a 0x00 0x05 0x02 0x00 0x00 0x01 0x00 0x8f 0x00 0x37 0x7e 0x00 0x00 0x00 0x01 0x04 0x00 0x00 0x00 0xff 0x02 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01 0xff 0x24 0x37 0xe8 
        */
       //-----------
       /*
       Terminal 2:
       $ ip monitor link 
        11: jitap: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN group default 
            link/ether 0e:3d:e2:24:37:e8 brd ff:ff:ff:ff:ff:ff
        11: jitap: <BROADCAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN group default 
            link/ether 0e:3d:e2:24:37:e8 brd ff:ff:ff:ff:ff:ff
        11: jitap: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UNKNOWN group default 
            link/ether 0e:3d:e2:24:37:e8 brd ff:ff:ff:ff:ff:ff
       */

       /* in output after 4 bytes (0x00 0x00 0x86 0xdd), we able to see dest mac address (0x33 0x33 0x00 0x00 0x00 0x16) then 
       src mac address (0x0e 0x3d 0xe2 0x24 0x37 0xe8), then protocol  (0x86 0xdd)

       so four bytes are prepended check /usr/include/linux/if_tun.h here, tun_pi comment says, if IFF_NO_PI is not set then 4 bytes will be prepended so add `IFF_NO_PI`
       */

    }
    return 0;

}
