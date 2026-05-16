## Socket Buffer

When a packet arrives at a network interface, it is raw bytes in hardware memory. The Linux kernel must process this packet through multiple subsystems — the NIC driver, Layer 2, Layer 3, Netfilter hooks, the routing subsystem, and finally either a local process or an outgoing interface.

Each subsystem needs to read different parts of the packet. 
* The IP layer reads the IP header. 
* TCP reads the TCP header. 
* Netfilter reads both.
* The routing system reads the destination IP.

All of these subsystems are separate pieces of code. They need a shared, structured object that represents the packet — one that all of them can read from and write to consistently.

`sk_buff` (socket buffer) is that shared object. It is a C structure defined in the Linux kernel that wraps a raw packet and carries it through the entire kernel network stack.

## Why Pointers Instead of Copies
A naive design would copy the packet data for each subsystem that needs to process it. The IP layer would get its own copy, Netfilter would get its own copy, and so on.

This would be catastrophically slow. A server processing 1 million packets per second, copying each packet 5 times, performs 5 million memory copy operations per second — each copying up to 1500 bytes.

`sk_buff` solves this with a single allocation. The packet data is allocated once in memory. Every subsystem receives a pointer to the same memory. No copying occurs. Each subsystem reads from the same buffer using its own pointer into that buffer.

```
ONE memory allocation:
+------------------------------------------+
|  [Ethernet header][IP header][TCP][Data] |
+------------------------------------------+
        ^               ^          ^
        |               |          |
   mac_header     network_header  transport_header
   (pointer)      (pointer)       (pointer)

NIC driver      sets mac_header
IP subsystem    sets network_header, reads dst IP
TCP subsystem   sets transport_header, reads port
Netfilter hook  reads both network_header and transport_header
Routing         reads network_header destination IP

All reading from the SAME memory. Zero copies.
```

## The Four Pointer Boundaries
```
Memory layout of an sk_buff's data buffer:

skb->head
    |
    v
    +---------+------------------+------------------+--------+
    | headroom|   PACKET DATA   |    tailroom       |        |
    |         | [hdrs + payload]|                   |        |
    +---------+------------------+------------------+--------+
    ^         ^                  ^                           ^
    |         |                  |                           |
  head      data               tail                        end

head  = start of allocated buffer (never moves)
data  = start of actual packet (moves when headers added/removed)
tail  = end of actual packet (moves when data appended)
end   = end of allocated buffer (never moves)

Packet length = tail - data = skb->len
Headroom      = data - head  (space to prepend headers)
Tailroom      = end  - tail  (space to append data)
```

**Why headroom exists**: When a packet moves down the stack toward the wire, each layer prepends its own header. The Ethernet driver prepends a 14-byte Ethernet header. If there is no headroom, the kernel must reallocate the entire buffer and copy everything. Headroom is pre-allocated space that avoids this reallocation.

**Why tailroom exists**: When a packet needs padding (below the 64-byte Ethernet minimum), zero bytes are appended. Tailroom provides space for this without reallocation.
