# Class D — Multicast Group Addressing



## 1. The Problem Class D Solves

To understand why Class D exists, first understand the two communication models that existed before it.

**Unicast** is one-to-one communication. One source sends a packet to one specific destination. Every device that wants the same data requires a separate, identical packet. If 1,000 devices want the same video stream, the source sends 1,000 identical packets — one per device. The network carries 1,000 copies of the same data simultaneously.

**Broadcast** is one-to-all communication. One source sends a packet to every device on a network segment, whether those devices want the data or not. Every device must receive and process the packet, then discard it if the data is not relevant. Broadcast does not cross routers — it is confined to a single local network.

Neither model is efficient for the following scenario: a source wants to send identical data to a specific group of devices scattered across multiple networks, where not every device on those networks wants the data.

- Unicast requires one packet per interested device. At 1,000 devices, that is 1,000 packets from the source.
- Broadcast sends to everyone, including uninterested devices, and cannot cross routers to reach other networks.

**Multicast solves this.** The source sends one packet to a group address. The network infrastructure replicates that packet only toward networks where at least one interested device exists. Devices that are not interested receive nothing.



## 2. What a Class D Address Is

A Class D address is a 32-bit IPv4 address whose first four bits are always `1110`.

```
First octet binary pattern:  1110xxxx
```

This gives the decimal range:

```
224.0.0.0   =  11100000.00000000.00000000.00000000
239.255.255.255 =  11101111.11111111.11111111.11111111
```

**Full Class D range: 224.0.0.0 to 239.255.255.255**

The remaining 28 bits (after the leading `1110`) form the Group ID. There are $2^{28}$ = 268,435,456 possible multicast group addresses.



## 3. No Network/Host Split

Classes A, B, and C each divide their 32 bits into two parts: a network identifier and a host identifier. Class D does not do this.

**Why there is no split:**

A unicast address must answer two questions: which network does this device belong to, and which specific device on that network is this? The network/host split exists to answer both questions.

A Class D address does not identify a device. It identifies a logical group — a label that means "anyone who has expressed interest in receiving this type of data." There is no single network that owns this address, and there is no single host that owns this address. The concept of a network/host boundary is therefore meaningless for Class D.

The entire 28 bits after the `1110` prefix are treated as a single, flat Group ID.

```
Class D address:
+-+--+--------------------------------------------+
|1110|         Group ID (28 bits)                 |
+-+--+--------------------------------------------+
```



## 4. The Three Subranges Within Class D

Not all Class D addresses behave the same way. The range is divided into three subranges, each with different forwarding rules.

```
224.0.0.0   – 224.0.0.255     Link-Local (reserved for control protocols)
224.0.1.0   – 238.255.255.255 Global scope (routable multicast)
239.0.0.0   – 239.255.255.255 Administratively scoped (private multicast)
```

### 4.1 Link-Local Range: 224.0.0.0 to 224.0.0.255

These addresses are reserved for network control protocols that operate only within a single local network segment. Routers never forward packets destined for these addresses, regardless of TTL value. The packet is confined to the local segment by design.

Examples of well-known link-local multicast addresses:

```
224.0.0.1   All hosts on the local segment
224.0.0.2   All routers on the local segment
224.0.0.5   All OSPF routers (a routing protocol)
224.0.0.6   OSPF designated routers
224.0.0.9   RIP version 2 routers
```

**Why this subrange exists:** Routing protocols need to communicate with neighboring routers or all local hosts without that traffic accidentally propagating across the entire internet. Fixing these addresses as non-routable guarantees confinement.

### 4.2 Global Scope Range: 224.0.1.0 to 238.255.255.255

These addresses are routable across the public internet. A multicast packet sent to an address in this range can, in principle, be forwarded by routers across network boundaries to reach interested subscribers anywhere in the world.

**Why this subrange exists:** Global multicast allows services like live internet broadcasts, financial market data feeds, and software distribution to reach subscribers worldwide from a single source.

### 4.3 Administratively Scoped Range: 239.0.0.0 to 239.255.255.255

This range is the multicast equivalent of private unicast addresses (`10.x.x.x`, `192.168.x.x`). These addresses are intended for use within a private organization. Routers at the boundary of an organization are configured to block these addresses from entering or leaving. Different organizations can use the same addresses in this range without conflict, because the addresses never leave each organization's network.

**Why this subrange exists:** An organization running internal multicast applications — a video conference system, internal software updates — needs addresses that will not collide with global multicast and will not accidentally leak to the public internet.



## 5. How a Device Uses a Class D Address

A Class D address is never assigned to a device as its identity. A device always keeps its unicast address (from Class A, B, or C) as its primary identifier.

### 5.1 The Subscription Model

A device interacts with Class D addresses through subscription, not ownership.

**Subscribing (Join):** A device that wants to receive data for a particular multicast group sends a signal to its local router: "this device wants to receive packets addressed to Group ID 224.x.x.x." The device does not change its IP address. It adds the group address as an additional address it listens for.

**Unsubscribing (Leave):** When a device no longer wants the data, it sends a leave signal to its local router. The router checks if any other device on that segment still wants the group. If none remain, the router stops forwarding that group's traffic to that segment.

### 5.2 Packet Fields During Multicast Communication

When a multicast packet travels across a network, its IP header fields are populated as follows:

```
Source IP address:      Unicast address of the sender
                        (e.g., 10.0.0.5)

Destination IP address: Class D Group address
                        (e.g., 224.1.2.3)

TTL:                    A number set by the sender to limit
                        how far the packet can travel
```

The source IP is always a unicast address. No device sends a multicast packet from a Class D source address. This allows routers receiving the packet to know exactly which device originated it.



## 6. TTL — Controlling How Far Multicast Travels

Every IP packet contains a TTL (Time To Live) field — an 8-bit number that starts at a value set by the sender. Each router that forwards the packet decrements this number by one. When TTL reaches zero, the router discards the packet and does not forward it further.

For multicast, TTL is used deliberately to define the geographic or administrative reach of a multicast stream.

```
TTL = 1    Packet stays on the local network segment.
           The first router decrements to 0 and discards.

TTL = 15   Packet travels within a site or campus.

TTL = 63   Packet travels within a region.

TTL = 127  Packet travels within a continent.

TTL = 255  Packet can travel globally without TTL restriction.
```

**Why TTL-based scoping exists:** A company running an internal video conference does not want that traffic leaking outside its network. By setting a low TTL, the sender ensures the packet cannot travel beyond a defined boundary without any need for complex firewall rules. The packet extinguishes itself.

**Important interaction with link-local addresses:** Packets destined for the link-local subrange (224.0.0.0 to 224.0.0.255) are never forwarded by routers regardless of TTL. TTL controls scope for global and administratively scoped ranges. For link-local, the address itself enforces confinement.



## 7. IGMP — The Protocol Between a Host and Its Router

Routers do not know automatically which devices on their segments want which multicast groups. A dedicated protocol handles this: IGMP (Internet Group Management Protocol).

IGMP operates only between a host and the router directly connected to its local network segment. It does not operate between routers.

### 7.1 The Join Operation

```
Host                        Local Router
 |                               |
 | IGMP Membership Report ------>|
 |    "I want group 224.1.2.3"   |
 |                               |
 |                   Router records: Port 3
 |                   has a subscriber for
 |                   group 224.1.2.3
```

1. A host decides it wants to receive a multicast group's data.
2. The host sends an IGMP Membership Report message to the multicast group address itself (not to the router's unicast address). All routers on the segment receive this.
3. The router records which physical port that host is connected to and associates it with the group address.

### 7.2 The Query — Router Checking for Active Members

A router does not assume that subscriptions last forever. At regular intervals, the router sends an IGMP Membership Query to `224.0.0.1` (all hosts on the segment).

```
Router                      All Hosts on Segment
 |                               |
 | IGMP Membership Query ------->|
 |    "Who still wants           |
 |     any multicast group?"     |
 |                               |
 |<-- Membership Reports --------|
 |    from still-interested hosts|
```

Hosts that still want a group reply with another Membership Report. Hosts that have lost interest remain silent. If no host replies for a particular group, the router removes that group from its record for that port and stops forwarding traffic there.

**Why periodic queries exist:** Devices can disconnect from a network without sending a formal Leave message — a power cut, a crash, a network cable pulled. Periodic queries ensure the router eventually discovers that a subscriber is gone, rather than forwarding data to an empty segment indefinitely.

### 7.3 The Leave Operation

```
Host                        Local Router
 |                               |
 | IGMP Leave Group ------------>|
 |    "I am leaving 224.1.2.3"   |
 |                               |
 |                   Router sends a
 |                   Group-Specific Query
 |                   to 224.1.2.3:
 |                   "Does anyone else
 |<-------------------till want this?"
 |                               |
 |----No replies/replies-------->|
 |                               |
 |                   Router prunes port.
 |                   Stops forwarding
 |                   224.1.2.3 here. Based on the replies
```

If another host on the same segment replies to the router's Group-Specific Query, the router keeps forwarding. The leaving device's unsubscription does not affect other subscribers on the same segment.

### 7.4 IGMP Versions

Three versions of IGMP have been defined. Each version is backward-compatible.

```
IGMPv1  (1989)  Basic join. No formal leave message.
                Router discovers departures only through
                query timeout.

IGMPv2  (1997)  Adds the Leave Group message.
                Reduces the time a router takes to detect
                a subscriber has left (from minutes to seconds).

IGMPv3  (2002)  Adds Source-Specific Multicast (SSM).
                A host can specify not just which group it
                wants, but which specific source it wants
                the data from.
                Example: "I want group 224.1.2.3 but only
                if the data comes from source 10.0.0.5."
                This prevents unwanted senders from injecting
                data into a group.
```



## 8. How Routers Forward Multicast Packets

IGMP tells a router which of its directly connected ports have subscribers. But a multicast packet must often travel through many routers across a large network before reaching those subscribers. This requires a separate mechanism: multicast routing.

### 8.1 The Multicast Forwarding Table

A router maintains a standard unicast routing table for normal traffic. For multicast, it maintains a separate structure: the Multicast Forwarding Table (also called the multicast routing table or MFC — Multicast Forwarding Cache).

Each entry in this table records:

```
(Source IP address, Group address) → list of outgoing interfaces
```

Example entry:

```
Source: 10.0.0.5
Group:  224.1.2.3
Incoming interface: eth0  (where the packet arrives from)
Outgoing interfaces: eth1, eth3  (where to send copies)
```

When a router receives a multicast packet, it looks up the (source, group) pair and sends a copy of the packet out of every listed outgoing interface.

### 8.2 Packet Replication

```
                     +-----------+
         eth0 ------>|           |-----> eth1 (subscribers present)
  (packet arrives)   |  Router   |
                     |           |-----> eth2 (no subscribers — pruned)
                     |           |
                     |           |-----> eth3 (subscribers present)
                     +-----------+

Router sends ONE copy out eth1, ONE copy out eth3.
eth2 receives nothing.
```

**Why replication happens at the router and not the source:** If the source sent individual copies to each subscriber, it would need to know every subscriber's unicast address and send one packet per subscriber. With multicast, the source sends one packet regardless of how many subscribers exist. Replication happens as close to the subscribers as possible, which minimizes the total amount of traffic on the network backbone.

### 8.3 Multicast Routing Protocols — Router-to-Router Coordination

IGMP only handles the relationship between a host and its directly connected router. A separate set of protocols coordinates multicast routing between routers. The most widely deployed is PIM (Protocol Independent Multicast).

PIM builds a distribution tree — a logical map of which routers need to receive and forward a particular multicast group's traffic. There are two tree models:

```
Shared Tree (RPT):
  All traffic for a group flows through a single
  central point called the Rendezvous Point (RP).
  Simpler to manage. Less optimal paths.

Source-Specific Tree (SPT):
  Each source has its own tree built directly from
  the source to all subscribers.
  More optimal paths. More state in routers.
```

**The boundary between IGMP and PIM:**

```
Host <IGMP> Local Router <PIM> Other Routers
       (local)                   (inter-router)
```

IGMP is entirely local — between a host and the one router on its segment. PIM is what makes multicast work across a large network of many routers. Both must function for end-to-end multicast delivery to work.



## 9. Complete Packet Flow — End to End

The following traces a single multicast packet from source to receiver across two routers.

```
[Source: 10.0.0.5]
       |
       | Sends one packet:
       | Src=10.0.0.5, Dst=224.1.2.3, TTL=64
       |
       v
[Router A]
       |-- Checks Multicast Forwarding Table
       |-- Entry found: (10.0.0.5, 224.1.2.3)
       |   Outgoing: eth1 (toward Router B)
       |             eth2 (local segment has subscribers)
       |
       |-- Decrements TTL to 63
       |-- Sends copy out eth1 → Router B
       |-- Sends copy out eth2 → Local subscribers
       |
       v
[Router B]
       |-- Receives packet from Router A, TTL=63
       |-- Checks Multicast Forwarding Table
       |-- Entry found: (10.0.0.5, 224.1.2.3)
       |   Outgoing: eth3 (local segment has subscribers)
       |             eth4 pruned (no subscribers)
       |
       |-- Decrements TTL to 62
       |-- Sends copy out eth3 only
       |
       v
[Subscribers on Router B's segment]
       Receive the packet.
```

At every step, only one copy travels between routers. Replication occurs at the last router before the subscriber segment. If no subscriber exists downstream of a router, no copy is sent in that direction.



## 10. Complete Reference Summary

```
Property                  Detail
  
Address range             224.0.0.0 to 239.255.255.255
Leading bits              1110xxxx
Group ID bits             28 bits (2^28 = 268 million possible groups)
Network/host split        None — entire address is a Group ID
Device ownership          Devices never own a Class D address
Device identity           Always a unicast address (Class A, B, or C)
Source IP in packet       Always the sender's unicast address
Destination IP in packet  The Class D Group ID
Scope control             TTL field (decremented at each router hop)
Host-router protocol      IGMP (Internet Group Management Protocol)
Router-router protocol    PIM (Protocol Independent Multicast)
Forwarding mechanism      Multicast Forwarding Table — replication per port
Pruning                   Ports with no subscribers receive no copies

Subrange                  Range                     Behavior
    
Link-local                224.0.0.0–224.0.0.255     Never forwarded by routers
Global scope              224.0.1.0–238.255.255.255  Routable on public internet
Administratively scoped   239.0.0.0–239.255.255.255  Private, org-internal use

IGMP version              Key addition
  
IGMPv1                    Basic join. No leave message.
IGMPv2                    Formal leave message. Faster departure detection.
IGMPv3                    Source-specific subscriptions (SSM).
```