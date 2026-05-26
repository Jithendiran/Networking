// gcc arp_responder.c -o /tmp/arp_responder
/*
* Author: E.K.Jithendiran
* Date  : 27.5.2026 
*/
/*
1. Open and up interface
2. From different terminal set the manual IP address `sudo ip addr add 10.0.0.1/24 dev jitap`
3. Listen to the incoming traffic on this interface
4. If a ARP resquest is received for it's own IP respond it with it's macaddress
--------------
5. Get it's MAC address
6. Refer: https://datatracker.ietf.org/doc/html/rfc826
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
#include <stdio.h>
int main(){
    return 0;
}