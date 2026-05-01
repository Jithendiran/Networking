//  gcc endian.c -o /tmp/endian.o
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <endian.h>

int main() {
    // 1. Port Number (16-bit)
    uint16_t host_port = 8080; // 0x1f90
    uint16_t net_port  = htons(host_port); // host to network 
    
    // 2. IP Address (32-bit)
    uint32_t host_ip = 0xc0a80101; // 192.168.1.1
    uint32_t net_ip  = htonl(host_ip);

    printf("Port (Host): 0x%04x | Port (Network): 0x%04x\n", host_port, net_port);
    printf("IP   (Host): 0x%08x | IP   (Network): 0x%08x\n", host_ip, net_ip);

    // Reverting back
    printf("Restored Port: %x\n", ntohs(net_port));


    uint64_t host_val = 0x0123456789ABCDEFULL;

    // Explicitly convert to Big-Endian (Network Order)
    uint64_t be_val = htobe64(host_val);
    
    // Explicitly convert to Little-Endian
    uint64_t le_val = htole64(host_val);  // in x86, no difference at all

    printf("Host:   0x%016lx\n", host_val);
    printf("Big:    0x%016lx\n", be_val);
    printf("Little: 0x%016lx\n", le_val);
    
    return 0;
}