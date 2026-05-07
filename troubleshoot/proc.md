```bash
localhost:~# ls /proc/net/
anycast6       if_inet6       mcfilter       raw            softnet_stat
arp            igmp           mcfilter6      raw6           stat
connector      igmp6          netfilter      route          tcp
dev            ip6_flowlabel  netlink        rt6_stats      tcp6
dev_mcast      ip6_mr_cache   netstat        rt_acct        udp
dev_snmp6      ip6_mr_vif     packet         rt_cache       udp6
fib_trie       ip_mr_cache    pnp            snmp           udplite
fib_triestat   ip_mr_vif      protocols      snmp6          udplite6
icmp           ipconfig       psched         sockstat       unix
icmp6          ipv6_route     ptype          sockstat6      xfrm_stat
```
# Reference: Analysis of /proc/net/ Components

The `/proc/net/` directory is a virtual interface provided by the Linux kernel. It allows users to view internal network statistics and configuration tables in real-time. Files within this directory are not stored on a disk; they are generated dynamically when read.


## 1. Primary Diagnostic Components
These components are essential for verifying basic connectivity and identifying the exact layer where a failure occurs.

### **dev** (Device Statistics)
* **Logic:** Tracks the total quantity of data (bytes and packets) passing through each network interface.
* **Purpose:** Verification of the **Physical and Data Link Layers**. If the `Receive` counter increments, the hardware is successfully receiving electrical or virtual signals.
* **Key Columns:** `bytes`, `packets`, `errs` (errors), and `drop` (packets discarded by the driver).

### **arp** (Address Resolution Protocol Table)
* **Logic:** Maps logical IP addresses to physical MAC addresses.
* **Purpose:** Verification of **Layer 2 (Data Link Layer)** connectivity. 
* **Failure Indicator:** If an entry exists but the MAC address is all zeros (`00:00:00:00:00:00`), the local node sent a request but the remote node did not respond.

### **route** (IPv4 Routing Table)
* **Logic:** Defines the path a packet must take based on its destination IP address.
* **Purpose:** Verification of the **Network Layer**. The kernel uses this to decide which interface to use for outgoing traffic.
* **Key Columns:** `Destination`, `Gateway`, `Flags` (U for Up, G for Gateway).

## 2. Protocol Stack Analysis
These files monitor the health of specific communication protocols.

### **snmp** (Simple Network Management Protocol counters)
* **Logic:** Accumulates statistics for every protocol layer (IP, ICMP, TCP, UDP).
* **Purpose:** Identifying "Silent Drops." 
* **Why it matters:** If `dev` shows packets arriving but `snmp` shows `InDelivers` is zero, the kernel is rejecting packets at the software level, often due to firewall rules or corrupted headers.

### **softnet_stat** (CPU Processing Stats)
* **Logic:** Tracks how the CPU handles network interrupts and packet queues.
* **Purpose:** Identifying performance bottlenecks. If the second column (drops) increments, the system is receiving data faster than the CPU can process it.


## 3. Communication Channel Monitoring
These files list active and passive communication end-points (sockets).

### **tcp / udp**
* **Logic:** Lists every open network socket on the system.
* **Purpose:** Verification of the **Transport Layer**. It shows which ports are "Listening" for connections and which are currently "Established."
* **Format:** Addresses are shown in Hexadecimal format (e.g., `0100A8C0` is `192.168.0.1`).

### **packet**
* **Logic:** Displays "Raw" sockets that bypass the standard TCP/IP stack.
* **Purpose:** Monitoring diagnostic tools. Programs like `tcpdump` or `wireshark` appear here because they capture data directly from the network driver.

## 4. Advanced Logic and Infrastructure
These components define the internal mathematical rules of the network stack.

### **fib_trie** (Forwarding Information Base)
* **Logic:** A specialized data structure (a "Trie") used to search the routing table efficiently.
* **Purpose:** Explaining *how* the kernel makes routing decisions. It organizes IP addresses into a tree-like structure to minimize search time.

### **ptype** (Packet Types)
* **Logic:** Lists the protocol handlers registered with the kernel.
* **Purpose:** If a packet type (like `0806` for ARP) is not listed here, the kernel is not programmed to understand that type of traffic and will ignore it.

### **dev_mcast** (Multicast Groups)
* **Logic:** Lists the multicast addresses an interface is currently joined to.
* **Purpose:** Essential for troubleshooting **QEMU Multicast Networking**. If the virtual wire uses `230.0.0.1`, the interface must be registered here to "hear" the traffic.