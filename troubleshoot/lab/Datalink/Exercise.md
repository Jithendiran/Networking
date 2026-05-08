## Exercise

This study examines a scenario where the **Physical Layer** is functional (data can be sent), but the **Link Layer** fails to establish a connection. This is observed when Node A broadcasts a request but receives total silence from the network.

## ARP failure
This is observed when Node A broadcasts a request but receives total silence from the network.

### Setup
By following these steps, a controlled failure is created to observe how the stack behaves when logical-to-physical mapping fails.
1.  **Preparation (Node B):** Disable ARP response logic: `ip link set dev eth0 arp off`.
2.  **Observation (Node A - Telnet Terminal):** Start the sniffer from `telnet` terminal: `tcpdump -i eth0 -e -n`.
    1. Start telnet `telnet localhost 4444` in a new terminal from host
    2. execute sniffer program: `tcpdump -i eth0 -e -n`
        - `-e`: By default, tcpdump only shows you Layer 3 (IP addresses) and above.  The `-e` flag forces it to show the Layer 2 (Link Layer) information.
        - `-n`: By default, tcpdump tries to be "helpful" by turning IP addresses and port numbers into names (e.g., converting 8.8.8.8 to google-public-dns-a.google.com). `-n` It disables "name resolution." It keeps 192.168.1.1 as 192.168.1.1.
3.  **Execution (Node A - Main Terminal):** 
    * Attempt to reach the peer by using ping command : `ping -c 2 192.168.1.2`.
    * While a standard ping uses ICMP (Layer 3), an arping sends ARP Requests (Layer 2). Many systems are configured to ignore `ICMP` "Echo Requests" (standard pings) for security. However, no host can communicate on an Ethernet network without responding to ARP. If ping fails but arping succeeds, the host is alive but hiding behind a firewall. command: `arping -I eth0 192.168.1.2`

### Observed Behavior & Logic

#### 1. Request sent from host

When the experiment is executed, the following symptoms appear:

*   **The Sniffer Output:** `tcpdump` shows outgoing frames with the broadcast destination `ff:ff:ff:ff:ff:ff`. The `ethertype` is `ARP (0x0806)`. No incoming frames appear.
*   **The Neighborhood Table:** Running `ip neigh show` on Node A results in an `INCOMPLETE` state for `192.168.1.2`.
*   **Logical Failure:** The `ping` command fails not because the destination is "down," but because Node A cannot determine which MAC address to put in the destination field of an ICMP (Ping) packet.

##### Root Cause Analysis: Why ARP Fails
When the physical carrier is functional but ARP replies are missing, the failure generally falls into two categories: **Environmental Mismatches** or **Node Presence Issues**.

##### 1. Environmental & Configuration Mismatches
Even if both nodes are "on," they may be logically isolated or confused by the network configuration.

*   **Multicast/Channel Isolation:** If QEMU nodes use different multicast addresses or ports, they are on different virtual "wires." A broadcast sent on Port 1234 will never be heard by a listener on Port 5555.
*   **VLAN Tagging:** If Node A encapsulates its ARP request in a **VLAN Tag** (802.1Q) that Node B is not configured to strip, Node B will drop the frame as "garbage" because the header format is unexpected.
*   **MAC Address Duplication:** If both nodes share the same MAC address, the network driver may drop incoming packets, assuming it is hearing its own "echo" rather than a legitimate response from a peer.

##### 2. Node Presence & Security Dropping
The request reaches the wire, but the target cannot or will not respond.

*   **The Target is Absent:** The simplest explanation; no hardware with that IP address is connected to the segment.
*   **Firewall/Filtering:** The target node receives the ARP request, but a security policy (such as `arptables` or `nftables`) drops the packet before the kernel can generate a reply.
*   **Administrative Suppression (`NOARP`):** As seen in our experiment, the interface may be configured to ignore the ARP protocol entirely. This is common in specialized "Point-to-Point" or "Static" network environments.

#### 2. Request is not sent from host 
Checks has to done form host
1.  **Routing Table Miss (No Route to Host)**
    Before a packet is created, the kernel consults the **Routing Table** to decide which interface to use. 
    *   **The Issue:** If the destination IP is `192.168.2.1` but your interface is configured as `192.168.1.1/24` and there is no **Default Gateway** defined, the kernel has no "path" to the destination.
    *   **The Logic:** The system refuses to build a packet if it doesn't know which "door" (interface) to send it through.
    *   **Check:** Run `ip route` or `route -n`. If there is no entry covering the destination IP, the request will be aborted immediately.

    ```bash
    localhost:~$ route -n
    Kernel IP routing table
    Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
    0.0.0.0         10.0.2.2        0.0.0.0         UG    203    0        0 eth1
    10.0.2.0        0.0.0.0         255.255.255.0   U     0      0        0 eth1
    192.168.1.0     0.0.0.0         255.255.255.0   U     0      0        0 eth0
    localhost:~$ ip route
    default via 10.0.2.2 dev eth1  metric 203 
    10.0.2.0/24 dev eth1 scope link  src 10.0.2.15 
    192.168.1.0/24 dev eth0 scope link  src 192.168.1.1 
    localhost:~$ 
    ```

    `192.168.1.0/24 dev eth0 scope link  src 192.168.1.1`
    - `192.168.1.0/24` (Destination Network)  Any packet destined for an IP between `192.168.1.1` and `192.168.1.254`
    - `dev eth0` (Output Interface)  This identifies the physical or virtual hardware "exit point."
    - `scope link` (Validity Range)
        **Logic:** In networking, "scope" refers to how far a destination is. `link` means the destination is physically (or virtually) on the same wire/segment.
        *   **Purpose:** It tells the kernel: "Do not look for a router. The destination is a neighbor. You must use **ARP** to find their MAC address directly."
    - `src 192.168.1.1` (Preferred Source Address) This flag forces the system to use `192.168.1.1` as its identity for these specific outgoing packets through this port.

    Our configurations are correct for `192.168.1.0`. Without this config, it would try to send that traffic to the Default Gateway `default` / `0.0.0.0`, which will drop the packet if it don't know how to route


4. **Local Firewall / Egress Filtering**
    Security software (like `iptables`, `nftables`, or `ebtables`) can be configured to "DROP" or "REJECT" outgoing packets.

    *   **The Issue:** An **OUTPUT** chain rule matches the request and blocks it before it hits the wire.
    *   **The Logic:** Firewalls act as a gatekeeper between the application and the network driver. If the rule says "Deny," the data is deleted in memory.
    *   **Check:** Run `iptables -L OUTPUT -v -n`. Look for "DROP" counts increasing during your request attempt.
        - if `iptables` not found check `cat /proc/net/snmp | grep Ip`
            * Analysis
            ```bash
            localhost:~# cat /proc/net/snmp | grep Ip
            Ip: Forwarding DefaultTTL InReceives InHdrErrors InAddrErrors ForwDatagrams InUnknownProtos InDiscards InDelivers OutRequests OutDiscards OutNoRoutes ReasmTimeout ReasmReqds ReasmOKs ReasmFails FragOKs FragFails FragCreates OutTransmits
            Ip: 2 64 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
            localhost:~# 
            ```
            - `InReceives`: 0: The kernel's standard IP stack has not yet processed any incoming IP packets. (Note: tcpdump may see packets that the IP stack hasn't counted yet because tcpdump hooks in at a lower level).
            - `InDiscards`: 0: This is the "Firewall Evidence" column. If a firewall (Netfilter) drops a packet, or if the kernel runs out of memory (buffer space), this counter increments. A value of 0 indicates that the kernel has not administratively rejected any data.
            - `InHdrErrors` / `InAddrErrors`: 0: No packets were dropped due to corrupted headers or incorrect destination addresses.

        - To know about the stats
        `cat /proc/net/dev`
        ```sh
        localhost:~# cat /proc/net/dev
        Inter-|   Receive                                                |  Transmit
        face |bytes    packets errs drop fifo frame compressed multicast|bytes    packd
          lo:       0       0    0    0    0     0          0         0        0     0
        eth0:    4046      89    0    0    0     0          0         0     1926     0
        ```
        as per the stats physical interaface is healthy
    - Enable local host port and ping self `ping -c 3 localhost`, if we got the result our system is healthy, no problems with firewall inside our computer

5. **Neighbour status**
    After ARP sent didn't get reply traffic
    ```
    localhost:~# ip neigh show
    192.168.1.2 dev eth0  used 0/0/0 probes 6 FAILED
    10.0.2.2 dev eth1 lladdr 52:55:0a:00:02:02 used 0/0/0 probes 4 STALE
    10.0.2.3 dev eth1 lladdr 52:55:0a:00:02:03 used 0/0/0 probes 4 STALE
    fe80::2 dev eth1 lladdr 52:56:00:00:00:02 router used 0/0/0 probes 0 STALE
    localhost:~# 

    ``` 
    If the status is `REACHABLE`, `STABLE` which is positive sign, but if status is `FAILED` no reply from `Node B`, if status is `INCOMPLETE`, still kernel retrying


### Node B trouble shoot

#### Simplest check

1. Run `tcpdump -i eth0 -e -n` in telnet for Node B, we can observe it is able to receive the request but this node not sending the request
    ```bash
    localhost:~# tcpdump -i eth0 -e -n
    tcpdump: verbose output suppressed, use -v[v]... for full protocol decode
    listening on eth0, link-type EN10MB (Ethernet), snapshot length 262144 bytes
    23:15:15.317396 52:54:00:12:34:01 > 33:33:00:00:00:02, ethertype IPv6 (0x86dd), length 70: fe80::5054:ff:fe12:3401 > ff02::2: ICMP6, router solicitation, length 16
    23:15:32.054505 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42: Request who-has 192.168.1.2 tell 192.168.1.1, length 28
    23:15:33.109818 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42: Request who-has 192.168.1.2 tell 192.168.1.1, length 28
    23:15:34.133341 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42: Request who-has 192.168.1.2 tell 192.168.1.1, length 28

    ```
#### Stats check
check procs [Refer](../../proc.md)
1. Are packets incrementing in the Receive column? `cat /proc/net/dev`, Are `tcpdump` shows the incoming packet?
    ```
    localhost:~# cat /proc/net/dev
    Inter-|   Receive                                                |  Transmit
    face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
        lo:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
    eth0:     798      11    0    0    0     0          0         0     1926      25    0    0    0     0       0          0
    eth1: 1483355    1038    0    0    0     0          0         0    31628     527    0    0    0     0       0          0
    ```

    If Yes (as seen in  output: 6 packets), Layer 1 and the Virtual Wire are functional. The data reached the hardware interface. The physical layer receiving signals
2. Are the packets seen in IP stats `cat /proc/net/snmp` (This is not for per interface, if we enabled other interface, then this check may not valid because other other's InReceives will increase, so temporarily down the interface `ip link set eth1 down`)
    ```
    localhost:~# cat /proc/net/snmp
    Ip: Forwarding DefaultTTL InReceives InHdrErrors InAddrErrors ForwDatagrams InUnknownProtos InDiscards InDelivers OutRequests OutDiscards OutNoRoutes ReasmTimeout ReasmReqds ReasmOKs ReasmFails FragOKs FragFails FragCreates OutTransmits
    Ip: 2 64 516 0 0 0 0 0 514 514 0 0 0 0 0 0 0 0 0 514
    ```

    InReceives: 516 will not increase further which indicates IP layer didn't received the traffic. So packets received at physical layer but dropped before reaching IP layer (network layer)

#### Why ARP didn't received at receiver side

##### 1. ARP disable
**Check**: ` ip addr`
```
localhost:~# ip addr
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <BROADCAST,MULTICAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP qlen 1000
    link/ether 52:54:00:12:34:02 brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.2/24 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::5054:ff:fe12:3402/64 scope link 
       valid_lft forever preferred_lft forever

```
Here it self `NOARP` is present, here problem is well known, "This interface does not participate in the ARP protocol at all".

**Solution**: `ip link set dev eth0 arp on`


## ARP enabled

Based on the provided data, the system exhibits a complete disconnect between the **Data Link Layer (Layer 2)** and the **Network Layer (Layer 3)**. The evidence shows that 3 packets were physically received by the interface (`eth0`), yet the IP stack reports zero `InReceives`.

When the `NOARP` flag is not present (meaning ARP is administratively enabled), but the IP layer remains blind to traffic, the investigation must focus on the protocols and security mechanisms that reside between the driver and the IP stack.


### Logic of Layer 2 to Layer 3 Packet Drops

The following table categorizes the reasons why packets reach the hardware but vanish before being counted by the IP stack.

| Logic Level | Component | Description and Reason for Drop |
| :--- | :--- | :--- |
| **Protocol Filtering** | **EtherType Mismatch** | The kernel only passes packets to the IP stack if the Ethernet header marks them as `IPv4 (0x0800)` or `IPv6`. If Node A is only sending `ARP (0x0806)` and they are not being replied to, the IP stack will never increment because ARP is not an IP packet. |
| **Security/Policy** | **arptables / nftables** | Specific Layer 2 firewalls can be configured to **DROP** incoming ARP requests. This prevents the kernel from ever generating a reply, stalling the connection before IP traffic can even be generated. |
| **Kernel Validation** | **Inbound Corruption** | If the checksum of the Ethernet frame is invalid, the driver discards the packet. This is logged in `cat /proc/net/dev` under the `errs` or `frame` columns. |
| **Address Validation** | **Martian Packets** | If the source IP or destination IP is considered "impossible" (e.g., 127.0.0.1 arriving on an external interface), the kernel drops it for security. This is often logged if `rp_filter` (Reverse Path Filtering) is active. |


# Detailed Reasons for the Observed Failure

### 1. ARP Request/Reply Lifecycle Failure (The "Half-Bridge")
In the provided scenario, Node A is sending **ARP Requests** to discover Node B. 
* **The "Why":** `cat /proc/net/snmp` only counts **IP packets**. ARP is a neighbor-discovery protocol that sits "below" IP. 
* **The Logic:** If Node B receives the ARP request but a firewall (like `arptables`) blocks the kernel from seeing it, Node B will never send an **ARP Reply**. Without the reply, Node A can never wrap an IP packet in a MAC header. Consequently, no actual IP traffic ever exists to be counted in `InReceives`.

### 2. Bridge or VLAN Mismatch
If the virtual environment (QEMU) is misconfigured, the packets might arrive with a **VLAN Tag** (802.1Q).
* **The "Why":** If the `eth0` interface on Node B is not configured to "untag" the traffic, the kernel sees the packet as an unknown protocol rather than standard IP/ARP.
* **The Logic:** The hardware interface sees the bits (`dev` increments), but since the protocol ID is shifted by the VLAN tag, the kernel does not know which software handler (IP or ARP) should receive the data.

### 3. Netfilter (Firewall) Drops
Linux uses the **Netfilter** framework to process packets. Rules can be set at the `PREROUTING` stage.
* **The "Why":** A rule such as `nft add rule bridge filter input ether type arp drop` would cause this exact symptom.
* **The Logic:** The firewall acts as a gatekeeper immediately after the driver receives the packet. If the rule says "Drop," the packet is deleted from memory. Because this happens before the packet is handed to the IP stack, the `snmp` counters remain at zero.

### 4. Hardware/Driver FIFO Overruns
While less likely in a virtual environment, if the system is under extreme stress, the `fifo` column in `cat /proc/net/dev` would increment.
* **The "Why":** The interface buffer is full.
* **The Logic:** The packet was "received" by the virtual hardware, but there was no room in the kernel's memory buffer to store it for the IP stack to read.


# Summary for Documentation
To conclude the analysis: The fact that `dev` increments while `snmp` does not, confirms that the communication is failing during **Address Resolution** or is being intercepted by a **Layer 2 filter**. The "Network" (IP) does not technically exist yet because the "Link" (MAC) has not been established.