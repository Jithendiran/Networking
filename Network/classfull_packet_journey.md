# Packet Transfer: Source to Destination — Complete Internal Journey

## Document Scope and Prerequisite Definitions

This document traces a single data packet from its origin device to its destination device. Every layer of that journey is examined: how the sending device obtains its address, how private addresses are converted to public addresses, how routers make forwarding decisions, and how the destination device receives and processes the packet.

Before the journey begins, four foundational mechanisms must be defined, because the packet's journey depends on all four.

## Part 1: Foundational Mechanisms

### 1.1 DHCP — Dynamic Host Configuration Protocol

#### The Problem DHCP Solves

Every device on a network requires four pieces of information before it can communicate:

1. Its own IP address
2. The subnet mask (defines the size of the local network)
3. The default gateway address (the router's local IP address — the exit point to other networks)
4. The DNS server address (not covered in this document)

Without a central system to distribute this information, a network administrator would have to manually configure every device. In a network of 500 computers, this is operationally impossible to maintain.

DHCP is a server-based protocol that automates this distribution. One device on the network (the DHCP server, which is often the router itself in home and small-office networks) holds a pool of available IP addresses and assigns them to requesting devices automatically.

#### The DHCP Process: Four-Step Exchange (DORA)

The exchange follows four steps, universally referred to as DORA.

```
Device (No IP yet)                    DHCP Server (192.168.1.1)
        |                                      |
Step 1  |------- DISCOVER (broadcast) -------->|
        |        src: 0.0.0.0                  |
        |        dst: 255.255.255.255          |
        |                                      |
Step 2  |<------- OFFER (broadcast) -----------|
        |        offered IP: 192.168.1.50      |
        |        lease time: 24 hours          |
        |                                      |
Step 3  |------- REQUEST (broadcast) --------> |
        |        "I accept 192.168.1.50"       |
        |                                      |
Step 4  |<------- ACKNOWLEDGE (broadcast) -----|
        |        "Confirmed. It is yours."     |
        |                                      |
      [Device configures itself with 192.168.1.50]
```

**Step 1 — DISCOVER:**
The device has no IP address. It identifies itself temporarily as `0.0.0.0` (the Class A reserved initialization address). Because it does not know the DHCP server's address, it cannot send the message directly. It broadcasts to `255.255.255.255` (the limited broadcast address), which reaches every device on the local physical segment. The message states: "Is there a DHCP server present? I need an IP address."

**Step 2 — OFFER:**
The DHCP server receives the broadcast. It selects an available IP address from its pool, marks it as temporarily reserved, and broadcasts an offer back. The offer contains the proposed IP address, subnet mask, default gateway address, and lease duration.

**Step 3 — REQUEST:**
The device broadcasts a formal request accepting the offered address. This step is broadcast (not sent directly to the server) because multiple DHCP servers may exist on the segment. The broadcast informs all servers that this specific offer was accepted, so other servers can release their temporary reservations.

**Step 4 — ACKNOWLEDGE:**
The DHCP server sends a final confirmation. The device now configures its network interface with the assigned values and becomes a fully operational participant on the network.

#### The Lease and Its Renewal

DHCP does not assign addresses permanently. Each assignment has a lease duration (commonly 24 hours for home networks, 8 hours for enterprise networks). At 50% of the lease duration, the device sends a unicast renewal request directly to the DHCP server. If the server confirms, the lease timer resets. If the server is unreachable, the device retries at 87.5% of the lease duration. If no renewal is confirmed before expiry, the device must restart the DORA process.

#### The Scope of DHCP

DHCP operates only within a single local network segment. A DHCP broadcast cannot cross a router. This is a deliberate design constraint — each network manages its own address pool independently. If a packet with destination `255.255.255.255` were allowed to cross routers, a single DHCP request could propagate across the entire internet.

---

### 1.2 APIPA — Automatic Private IP Addressing

#### When APIPA Activates

APIPA activates under one specific condition: a device completes the DHCP DISCOVER step, waits the standard timeout period (approximately 60 seconds by default on most operating systems), receives no OFFER response, and concludes that no DHCP server is reachable.

At this point, rather than leaving the device with no address at all, the operating system's TCP/IP stack activates the APIPA fallback routine.

#### The Self-Assignment Process

```
Device (DHCP failed)
        |
        |-- Sends DISCOVER
        |-- Waits ~60 seconds
        |-- No response received
        |
        |--> APIPA activates
        |
        |-- Randomly selects address from 169.254.1.0 to 169.254.254.255
        |   (169.254.0.0 and 169.254.255.255 are reserved within the range)
        |
        |-- Sends ARP probe: "Does anyone own 169.254.x.x?"
        |
        |-- If conflict detected  --> selects a different random address, repeats
        |-- If no conflict        --> assigns the address to the interface
        |
        |--> Device is now operational on 169.254.x.x / 255.255.0.0
        |
        |-- Background DHCP retry continues every 5 minutes
```

#### What APIPA Provides and What It Cannot Provide

APIPA assigns an address and a subnet mask (`255.255.0.0`). It does not and cannot assign a default gateway, because no router is involved in the process. The consequence is absolute:

- A device on APIPA **can** communicate with other devices on the same physical segment that are also using APIPA addresses.
- A device on APIPA **cannot** communicate with any device outside the local segment. There is no gateway — no path to leave the local network.

APIPA is therefore a limited-operation mode, not a functional replacement for DHCP.

---

### 1.3 NAT — Network Address Translation

#### The Core Problem NAT Solves

Private IP addresses (10.x.x.x, 172.16.x.x–172.31.x.x, 192.168.x.x) are non-routable. A packet with a private source address sent onto the public internet will be dropped by the first public router it encounters. ISPs and backbone routers are programmed to discard traffic carrying private addresses.

However, organizations use private addresses internally because public IP addresses are a finite and expensive resource. A company with 500 internal devices cannot obtain 500 public IP addresses. NAT is the mechanism that resolves this conflict.

#### How NAT Works: The Translation Table

The NAT device (always the border router — the router at the boundary between the private internal network and the public internet) maintains a translation table in memory. This table maps internal private address + port combinations to the router's single public IP address + a unique port number.

```
NAT Translation Table (stored in border router memory)

| Internal IP    | Int. Port | Public IP      | Pub. Port | Protocol | State    |
|----------------|-----------|----------------|-----------|----------|----------|
| 192.168.1.50   | 54231     | 203.0.114.5    | 40001     | TCP      | ACTIVE   |
| 192.168.1.51   | 48920     | 203.0.114.5    | 40002     | TCP      | ACTIVE   |
| 192.168.1.75   | 61200     | 203.0.114.5    | 40003     | TCP      | ACTIVE   |
```

All three internal devices share one public IP address (`203.0.114.5`), differentiated only by the unique port number assigned to each session.

#### The Outbound Translation (Private → Public)

When a packet leaves the internal network heading toward the internet:

1. The packet arrives at the border router with source address `192.168.1.50:54231`.
2. The router creates a new entry in the NAT table, mapping `192.168.1.50:54231` to `203.0.114.5:40001`.
3. The router rewrites the packet header — replacing the private source address with the public source address.
4. The modified packet is forwarded onto the internet.

The destination server on the internet sees the packet as originating from `203.0.114.5:40001`. It has no knowledge of the internal device `192.168.1.50`.

#### The Inbound Translation (Public → Private)

When the response packet returns from the internet:

1. The packet arrives at the border router addressed to `203.0.114.5:40001`.
2. The router consults the NAT table. Port `40001` maps to internal device `192.168.1.50:54231`.
3. The router rewrites the destination address from `203.0.114.5:40001` to `192.168.1.50:54231`.
4. The packet is forwarded onto the internal network to reach the correct device.

Without the NAT table, the router would have no way to determine which internal device a returning packet belongs to.

---

### 1.4 The Role of ARP — Address Resolution Protocol

#### Why ARP Is Required

IP addresses identify devices logically. However, data physically moves across a network using hardware addresses — specifically, MAC (Media Access Control) addresses burned into every network interface card. Before a device can send a packet to another device on the same local network, it must determine the MAC address that corresponds to the destination IP address.

ARP performs this resolution.

#### The ARP Process

```
Device A (192.168.1.50)          Device B (192.168.1.1 — the router)
        |                                      |
        |-- Checks ARP cache                   |
        |-- No entry for 192.168.1.1           |
        |                                      |
        |-- ARP Request (broadcast)----------->|  (to all devices on segment)
        |   "Who has 192.168.1.1?              |
        |    Tell 192.168.1.50"                |
        |                                      |
        |<-- ARP Reply (unicast) --------------|
        |    "192.168.1.1 is at                |
        |     MAC: AA:BB:CC:DD:EE:FF"          |
        |                                      |
        |-- Stores in ARP cache                |
        |-- Now sends packet to router         |
```

ARP results are cached for a period (typically 2–20 minutes depending on the operating system) to avoid repeating the lookup for every packet.

---

## Part 2: The Network Topology

> Company bought class C IP address, internally they can be use class A,B,C private IP's

The following topology is used for the complete trace. Every device, address, and network boundary is defined here so the journey can be followed precisely.

```
INTERNAL NETWORK A (Office — Source)
=====================================
 [Device A]
  Private IP  : 192.168.1.50 / 255.255.255.0  (Class C, assigned by DHCP)
  Gateway     : 192.168.1.1
  MAC         : AA:AA:AA:AA:AA:01

 [Office Router / NAT Device / DHCP Server]
  LAN-side IP : 192.168.1.1  (private, Class C)
  WAN-side IP : 203.0.114.5  (public, Class C — assigned by ISP)
  MAC (LAN)   : AA:AA:AA:AA:AA:02


PUBLIC INTERNET (ISP Backbone)
================================
 [ISP Router A]       — 198.51.100.1
 [ISP Core Router]    — 198.51.100.9
 [ISP Router B]       — 198.51.101.1


INTERNAL NETWORK B (Destination Office)
==========================================
 [Destination Router / NAT Device]
  WAN-side IP : 203.0.115.8  (public, Class C — assigned by ISP)
  LAN-side IP : 10.0.0.1     (private, Class A)
  MAC (LAN)   : BB:BB:BB:BB:BB:01

 [Device Z]
  Private IP  : 10.0.0.25 / 255.0.0.0  (Class A, assigned by DHCP)
  Gateway     : 10.0.0.1
  MAC         : BB:BB:BB:BB:BB:02
```

**What Device A knows:** Its own private IP, its gateway's IP, and the destination's public IP address (`203.0.115.8`). Device A does not know and does not need to know Device Z's private address. The connection is initiated to the public-facing address of Network B's router.

> **Note on addressing:** In classful addressing, `192.168.1.0` is a Class C network (first three octets = network, last octet = host). `10.0.0.0` is a Class A network (first octet = network, last three octets = host). Both are private ranges, non-routable on the public internet.

---

## Part 3: Pre-Journey — Device A Obtains Its Address

Before any packet can be sent to Device Z, Device A must have a valid IP address. The following sequence occurs when Device A joins the network.

### Step 3.1 — Device A Has No Address (Initialization State)

Device A's network interface card is active, but no IP address has been assigned. The TCP/IP stack initializes with the temporary source address `0.0.0.0`.

### Step 3.2 — DHCP DORA Exchange

```
Device A (0.0.0.0)                    Office Router (192.168.1.1)
                                       acting as DHCP Server
        |                                      |
        |--- DISCOVER ------------------------>|
        |    src IP  : 0.0.0.0                 |
        |    dst IP  : 255.255.255.255         |
        |    src MAC : AA:AA:AA:AA:AA:01       |
        |    "I need an IP address"            |
        |                                      |
        |<-- OFFER ----------------------------|
        |    Offered IP    : 192.168.1.50      |
        |    Subnet Mask   : 255.255.255.0     |
        |    Gateway       : 192.168.1.1       |
        |    Lease Duration: 24 hours          |
        |                                      |
        |--- REQUEST ------------------------->|
        |    "I accept 192.168.1.50"           |
        |                                      |
        |<-- ACKNOWLEDGE ----------------------|
        |    "192.168.1.50 is confirmed yours."|
        |                                      |
   [Device A configures interface]
   IP      : 192.168.1.50
   Mask    : 255.255.255.0
   Gateway : 192.168.1.1
```

Device A is now a functioning network participant. The DHCP exchange used private Class C addresses within the local segment only. No part of this exchange left the local network.

### Step 3.3 — What If DHCP Had Failed? (APIPA Scenario)

If the office router's DHCP service had been offline during the above step, the following would have occurred instead:

```
Device A (0.0.0.0)
        |
        |--- DISCOVER (broadcast) --> [no response after ~60 seconds]
        |
        |--> APIPA activates
        |
        |    Randomly selects: 169.254.47.22
        |    ARP probe: "Does anyone own 169.254.47.22?" --> No conflict
        |
        |    Assigns: 169.254.47.22 / 255.255.0.0
        |    Gateway: NONE
        |
        |--> Device A can only communicate with other 169.254.x.x
        |    devices on the same physical segment.
        |    Device A cannot reach the internet or any remote network.
        |
        |--> Background DHCP retry every 5 minutes
        |    When DHCP server recovers, Device A discards APIPA address
        |    and restarts DORA to obtain a proper address.
```

The packet journey to Device Z is impossible from an APIPA state. The remainder of this document assumes DHCP succeeded and Device A holds `192.168.1.50`.

> A single router connects five host devices and one DHCP server. Three host devices successfully received private IP addresses from the DHCP server. Two host devices failed to receive addresses from the server and assigned themselves APIPA (Automatic Private IP Addressing) addresses.  
>  
> Devices with private IP addresses communicate with each other within the local network segment. These devices access external networks and the internet through the router. Communication is not possible between private IP devices and APIPA-assigned devices.
> 
> Devices with APIPA addresses only communicate with each other. APIPA devices cannot connect to the three devices holding private IP addresses. Access to external networks or the internet is unavailable for APIPA devices.

---

## Part 4: Device A Prepares the Packet

Device A's application generates data that must be sent to Device Z. The destination is known as the public IP address of Network B's router: `203.0.115.8`.

### Step 4.1 — Destination Network Determination

Before constructing the packet, Device A's TCP/IP stack applies the subnet mask to determine whether the destination is on the local network or on a remote network.

```
Device A's IP address    : 192.168.1.50
Subnet Mask              : 255.255.255.0

Network of Device A:
  192.168.1.50  AND  255.255.255.0  =  192.168.1.0

Destination IP           : 203.0.115.8
  203.0.115.8   AND  255.255.255.0  =  203.0.115.0

192.168.1.0  ≠  203.0.115.0
```

The destination is on a different network. Device A cannot deliver this packet directly. The packet must be forwarded to the default gateway (`192.168.1.1`) for routing.

### Step 4.2 — ARP Resolution for the Gateway

Device A needs the MAC address of the gateway (`192.168.1.1`) to construct the data link layer frame. Device A checks its ARP cache.

```
ARP Cache (Device A):
  [empty — first transmission after startup]
```

No entry exists. Device A initiates an ARP request.

```
ARP Request (broadcast to all devices on 192.168.1.0 network):
  "Who has 192.168.1.1? Tell 192.168.1.50"
  src MAC: AA:AA:AA:AA:AA:01
  dst MAC: FF:FF:FF:FF:FF:FF  (broadcast)

ARP Reply (from Office Router):
  "192.168.1.1 is at AA:AA:AA:AA:AA:02"

Device A stores in ARP cache:
  192.168.1.1  -->  AA:AA:AA:AA:AA:02
```

### Step 4.3 — Packet Construction

Device A now constructs the complete packet:

```
PACKET HEADER at this stage
-----------------------------
Source IP      : 192.168.1.50      (Device A's private IP)
Destination IP : 203.0.115.8       (Network B router's public IP)
Source MAC     : AA:AA:AA:AA:AA:01 (Device A's MAC)
Destination MAC: AA:AA:AA:AA:AA:02 (Office Router's LAN-side MAC)
```

A critical observation: **the IP addresses identify the ultimate source and destination across the entire journey. The MAC addresses identify only the next physical hop.** MAC addresses change at every router. IP addresses remain constant until NAT modifies them.

---

## Part 5: The Packet Crosses the Internal Network (Private Zone)

### Step 5.1 — Packet Travels from Device A to the Office Router

The packet travels across the physical medium of Network A (ethernet cable or wireless) from Device A to the office router. This is a single physical hop within the private network.

```
Physical Hop 1:
  Device A  (192.168.1.50)
      |
      |  [Frame contains]
      |  src IP  : 192.168.1.50   <-- private
      |  dst IP  : 203.0.115.8    <-- public (final destination)
      |  src MAC : AA:AA:AA:AA:AA:01
      |  dst MAC : AA:AA:AA:AA:AA:02
      |
      v
  Office Router LAN interface (192.168.1.1)
```

The office router receives the frame on its LAN-side interface. It strips the data link layer frame (the MAC address wrapper) to examine the IP packet inside.

---

## Part 6: NAT — Private Address Converted to Public Address

### Step 6.1 — The Router Examines the Packet

The office router's routing engine examines the destination IP address: `203.0.115.8`. The router consults its routing table.

```
Office Router Routing Table:
  192.168.1.0/255.255.255.0  --> LAN interface (local delivery)
  0.0.0.0/0.0.0.0            --> WAN interface via ISP link  (default route)
```

`203.0.115.8` does not match the local network. It matches the default route (the catch-all entry for all non-local destinations). The packet must exit through the WAN interface toward the ISP.

### Step 6.2 — Outbound NAT Translation

Before forwarding the packet, the router's NAT engine executes the address translation. The source address `192.168.1.50` is a private address and will be rejected by public internet routers. It must be replaced with the router's public WAN-side IP address.

```
BEFORE NAT (packet as received from Device A):
  src IP  : 192.168.1.50 : 54231   <-- private, non-routable
  dst IP  : 203.0.115.8  : 443

NAT OPERATION:
  Router creates entry in NAT table:
  | 192.168.1.50 | 54231 | 203.0.114.5 | 40001 | TCP | ACTIVE |

  Router rewrites packet header:
  src IP  : 192.168.1.50 : 54231  -->  203.0.114.5 : 40001

AFTER NAT (packet as it enters the public internet):
  src IP  : 203.0.114.5  : 40001   <-- public, routable
  dst IP  : 203.0.115.8  : 443
```

The packet's internal origin (`192.168.1.50`) is now hidden. From the perspective of every router and device on the public internet, this packet originated from `203.0.114.5`.

### Step 6.3 — New MAC Addresses for the Next Hop

The router now needs the MAC address of the next device on the path: ISP Router A (`198.51.100.1`). The router performs ARP on the WAN-side link, obtains the MAC address, and reconstructs the data link frame.

```
Physical Hop 2 (exiting the private network):
  Office Router WAN interface (203.0.114.5)
      |
      |  [Frame contains]
      |  src IP  : 203.0.114.5    <-- public (NAT applied)
      |  dst IP  : 203.0.115.8    <-- public
      |  src MAC : [Office Router WAN MAC]
      |  dst MAC : [ISP Router A MAC]
      |
      v
  ISP Router A (198.51.100.1)
```

The packet has now exited the private network entirely. Private addresses are no longer present in any header field.

---

## Part 7: The Packet Traverses the Public Internet

Every router on the public internet performs the same three-step operation: receive the frame, strip the data link layer, consult the routing table, determine the next hop, perform ARP for the next hop's MAC address, reconstruct the frame, and forward.

The IP header (source `203.0.114.5`, destination `203.0.115.8`) does not change across any public internet hop. Only the MAC addresses change at each router, because MAC addresses are local-link identifiers, not end-to-end identifiers.

### Step 7.1 — ISP Router A

```
ISP Router A (198.51.100.1)
  Receives packet from Office Router
  Examines destination: 203.0.115.8
  Consults routing table --> next hop: ISP Core Router (198.51.100.9)
  ARP resolves 198.51.100.9 MAC
  Forwards packet

Physical Hop 3:
  ISP Router A  -->  ISP Core Router
  src IP : 203.0.114.5  (unchanged)
  dst IP : 203.0.115.8  (unchanged)
  src MAC: [ISP Router A MAC]       <-- new MAC for this hop
  dst MAC: [ISP Core Router MAC]    <-- new MAC for this hop
```

### Step 7.2 — ISP Core Router

```
ISP Core Router (198.51.100.9)
  Receives packet from ISP Router A
  Examines destination: 203.0.115.8
  Consults routing table --> next hop: ISP Router B (198.51.101.1)
  ARP resolves 198.51.101.1 MAC
  Forwards packet

Physical Hop 4:
  ISP Core Router  -->  ISP Router B
  src IP : 203.0.114.5  (unchanged)
  dst IP : 203.0.115.8  (unchanged)
  src MAC: [ISP Core Router MAC]    <-- new MAC for this hop
  dst MAC: [ISP Router B MAC]       <-- new MAC for this hop
```

### Step 7.3 — ISP Router B

```
ISP Router B (198.51.101.1)
  Receives packet from ISP Core Router
  Examines destination: 203.0.115.8
  Consults routing table --> next hop: Destination Router (203.0.115.8)
  ARP resolves 203.0.115.8 MAC
  Forwards packet

Physical Hop 5:
  ISP Router B  -->  Destination Router WAN interface
  src IP : 203.0.114.5  (unchanged)
  dst IP : 203.0.115.8  (unchanged)
  src MAC: [ISP Router B MAC]             <-- new MAC for this hop
  dst MAC: [BB:BB:BB:BB:BB:01]   <-- new MAC for this hop
```

---

## Part 8: The Packet Enters Network B — Inbound NAT

### Step 8.1 — Destination Router Receives the Packet

The destination router (Network B's border router) receives the packet on its WAN interface. Its WAN-side IP address is `203.0.115.8`, which matches the packet's destination IP. The router accepts the packet.

The router's NAT engine is now consulted. This is an inbound packet arriving at the public IP of the router. The router must determine which internal device this packet is intended for.

### Step 8.2 — Port Forwarding / Pre-established Session Lookup

In a classful network context, the destination router can direct inbound traffic to an internal device in one of two ways:

**Scenario A — Response to an outgoing session (most common):**
If Device Z had previously initiated a connection outward (and therefore an entry exists in the NAT table), the router finds that entry and translates the destination address.

**Scenario B — Port forwarding (for inbound connections to a server):**
If this packet is a new inbound connection (Device A is connecting to a service running on Device Z), the network administrator must have pre-configured a port forwarding rule on the destination router:

```
Port Forwarding Rule on Destination Router:
  Any packet arriving at 203.0.115.8 on port 443
  --> forward to internal address 10.0.0.25 port 443
```

```
INBOUND NAT OPERATION:

BEFORE NAT (packet as received from internet):
  src IP  : 203.0.114.5 : 40001    <-- public (Network A's router)
  dst IP  : 203.0.115.8 : 443      <-- public (this router's WAN IP)

NAT OPERATION:
  Router consults port forwarding table:
  port 443 --> 10.0.0.25 : 443

  Router rewrites destination address:
  dst IP  : 203.0.115.8 : 443  -->  10.0.0.25 : 443

AFTER NAT (packet as it enters the private network):
  src IP  : 203.0.114.5 : 40001    <-- public (still unchanged)
  dst IP  : 10.0.0.25   : 443      <-- private (Class A, now routable internally)
```

The packet now carries a private Class A destination address. It can be forwarded within Network B's internal network.

---

## Part 9: The Packet Crosses Network B's Internal Network (Private Zone)

### Step 9.1 — Router Determines Internal Delivery Path

The destination router examines the new destination: `10.0.0.25`. It consults its routing table.

```
Destination Router Routing Table:
  10.0.0.0/255.0.0.0  --> LAN interface (local delivery)
  0.0.0.0/0.0.0.0     --> WAN interface (default route to internet)
```

`10.0.0.25` matches the local network entry. The packet is destined for a device directly on the LAN.

### Step 9.2 — ARP Resolution for Device Z

The router needs Device Z's MAC address to construct the delivery frame.

```
ARP Request (from Destination Router, broadcast on 10.0.0.0 network):
  "Who has 10.0.0.25? Tell 10.0.0.1"
  dst MAC: FF:FF:FF:FF:FF:FF (broadcast)

ARP Reply (from Device Z):
  "10.0.0.25 is at BB:BB:BB:BB:BB:02"
```

### Step 9.3 — Final Packet Delivery to Device Z

```
Physical Hop 6 (final hop — inside Network B's private space):
  Destination Router LAN interface (10.0.0.1)
      |
      |  [Frame contains]
      |  src IP  : 203.0.114.5    <-- public (Network A's router, unchanged)
      |  dst IP  : 10.0.0.25      <-- private (NAT applied at entry)
      |  src MAC : BB:BB:BB:BB:BB:01  (Destination Router LAN MAC)
      |  dst MAC : BB:BB:BB:BB:BB:02  (Device Z MAC)
      |
      v
  Device Z (10.0.0.25)
```

Device Z receives the frame. Its network interface confirms the destination MAC address matches its own. It accepts the frame, strips the data link layer, and passes the IP packet up to the TCP/IP stack for processing.

---

## Part 10: The Return Journey (Response Packet)

Device Z processes the request and sends a response. The return path follows the same mechanism in reverse.

### Step 10.1 — Device Z Constructs the Response

```
Device Z builds response packet:
  src IP  : 10.0.0.25    : 443     <-- its own private IP
  dst IP  : 203.0.114.5  : 40001   <-- the source from the received packet
```

Device Z does not know about the NAT translation. From its perspective, the request arrived from `203.0.114.5` and the response is sent back to `203.0.114.5`. This is the designed behavior of NAT — internal devices are shielded from the full picture.

### Step 10.2 — Outbound NAT at Network B's Router

```
Device Z  -->  Destination Router (10.0.0.1)

Destination Router NAT operation:
  src IP  : 10.0.0.25   : 443  -->  203.0.115.8 : 443

Packet exits Network B:
  src IP  : 203.0.115.8 : 443    <-- public
  dst IP  : 203.0.114.5 : 40001  <-- public
```

### Step 10.3 — Public Internet Transit (Return Path)

The response packet traverses the ISP backbone in the reverse direction. At each router, MAC addresses are updated for the specific hop, but IP addresses remain unchanged:

```
ISP Router B     -->  ISP Core Router  -->  ISP Router A  -->  Office Router WAN
(each hop updates MAC addresses only; IP headers unchanged)
```

### Step 10.4 — Inbound NAT at the Office Router (Network A)

```
Office Router receives response:
  src IP  : 203.0.115.8 : 443
  dst IP  : 203.0.114.5 : 40001

NAT table lookup:
  Port 40001  -->  192.168.1.50 : 54231

NAT operation:
  dst IP  : 203.0.114.5 : 40001  -->  192.168.1.50 : 54231

Packet forwarded to Device A:
  src IP  : 203.0.115.8 : 443    <-- public (Network B's router)
  dst IP  : 192.168.1.50 : 54231 <-- private (Device A)
```

### Step 10.5 — Final Delivery to Device A

The office router performs ARP for Device A (or uses its cached entry), constructs the final frame, and delivers the response to Device A on the `192.168.1.0` local network.

Device A receives the response. The round trip is complete.

---

## Part 11: Complete Journey Summary

```
=============================================================
COMPLETE PACKET JOURNEY: Device A --> Device Z
=============================================================

PHASE 1: SETUP (before first packet)
  Device A  <--> Office Router       DHCP DORA exchange
                                     Device A receives 192.168.1.50

  IF DHCP FAILED:
  Device A self-assigns 169.254.x.x  (APIPA)
  Journey to Device Z is IMPOSSIBLE from this state

-------------------------------------------------------------
PHASE 2: PACKET LEAVES DEVICE A (private zone)

  Hop 1:  Device A          -->  Office Router (LAN side)
          src IP: 192.168.1.50 (PRIVATE, Class C)
          dst IP: 203.0.115.8  (public)
          src MAC: AA:AA:AA:AA:AA:01
          dst MAC: AA:AA:AA:AA:AA:02

-------------------------------------------------------------
PHASE 3: NAT TRANSLATION (private --> public)

  At Office Router:
    192.168.1.50:54231  mapped to  203.0.114.5:40001
    Packet header rewritten

  Hop 2:  Office Router (WAN) -->  ISP Router A
          src IP: 203.0.114.5  (PUBLIC, Class C)
          dst IP: 203.0.115.8  (public)
          [Private address 192.168.1.50 is no longer visible]

-------------------------------------------------------------
PHASE 4: PUBLIC INTERNET TRANSIT

  Hop 3:  ISP Router A     -->  ISP Core Router
  Hop 4:  ISP Core Router  -->  ISP Router B
  Hop 5:  ISP Router B     -->  Destination Router (WAN)

  At every hop:
    src IP: 203.0.114.5  (unchanged)
    dst IP: 203.0.115.8  (unchanged)
    MAC addresses: updated at each hop for local delivery only

-------------------------------------------------------------
PHASE 5: NAT TRANSLATION (public --> private, at destination)

  At Destination Router:
    Port forwarding rule: port 443 --> 10.0.0.25
    Packet header rewritten
    dst IP: 203.0.115.8 --> 10.0.0.25

-------------------------------------------------------------
PHASE 6: PACKET DELIVERED INSIDE NETWORK B (private zone)

  Hop 6:  Destination Router (LAN) --> Device Z
          src IP: 203.0.114.5  (public — Network A's router)
          dst IP: 10.0.0.25    (PRIVATE, Class A)
          src MAC: BB:BB:BB:BB:BB:01
          dst MAC: BB:BB:BB:BB:BB:02

  Device Z receives and processes the packet.

=============================================================
ADDRESS TYPE USED AT EACH ZONE
=============================================================

  [Device A]--[Office Router LAN]    : PRIVATE  (192.168.x.x, Class C)
  [Office Router WAN]--[Internet]    : PUBLIC   (203.0.114.5, Class C)
  [Internet]--[Dest. Router WAN]     : PUBLIC   (203.0.115.8, Class C)
  [Dest. Router LAN]--[Device Z]     : PRIVATE  (10.x.x.x,   Class A)

  APIPA (169.254.x.x, Class B):      Emergency fallback only.
                                     Active only when DHCP fails.
                                     Cannot participate in this journey.

=============================================================
WHAT CHANGED AT EACH ROUTER
=============================================================

  Office Router     : IP rewritten (NAT out), MAC rewritten (new hop)
  ISP Router A      : MAC rewritten only
  ISP Core Router   : MAC rewritten only
  ISP Router B      : MAC rewritten only
  Destination Router: IP rewritten (NAT in), MAC rewritten (new hop)
```

---

## Part 12: Key Principles Derived from the Journey

**Private addresses exist only within bounded network segments.** The address `192.168.1.50` is meaningful only within Network A. The address `10.0.0.25` is meaningful only within Network B. Neither address ever appears in a packet header while that packet is on the public internet.

**Public addresses are the identity of the border router, not the internal device.** Device A has no public IP address of its own. Its traffic is represented on the public internet by the router's public IP. The port number is the mechanism that distinguishes between multiple devices sharing that one public IP.

**MAC addresses are hop-specific. IP addresses are end-to-end.** This distinction is absolute. The MAC address on a frame at Hop 3 has no connection to the MAC address on a frame at Hop 1. The IP addresses in the packet header, however, persist from source to destination (with the exception of NAT modification at the border routers).

**DHCP is the prerequisite for all normal network operation.** Without a successful DHCP exchange, a device operates only in APIPA mode — isolated to its local physical segment with no path to any remote destination.

**NAT is not a routing protocol.** NAT does not decide where to send packets. Routing decides where to send packets. NAT only rewrites addresses to allow private-network traffic to travel on a public infrastructure and return to the correct internal device.