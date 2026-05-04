## Exercise

This study examines a scenario where the **Physical Layer** is functional (data can be sent), but the **Link Layer** fails to establish a connection. This is observed when Node A broadcasts a request but receives total silence from the network.

### ARP failure
This is observed when Node A broadcasts a request but receives total silence from the network.

#### Setup
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

#### Observed Behavior & Logic

##### 1. Request sent from host

When the experiment is executed, the following symptoms appear:

*   **The Sniffer Output:** `tcpdump` shows outgoing frames with the broadcast destination `ff:ff:ff:ff:ff:ff`. The `ethertype` is `ARP (0x0806)`. No incoming frames appear.
*   **The Neighborhood Table:** Running `ip neigh show` on Node A results in an `INCOMPLETE` state for `192.168.1.2`.
*   **Logical Failure:** The `ping` command fails not because the destination is "down," but because Node A cannot determine which MAC address to put in the destination field of an ICMP (Ping) packet.

###### Root Cause Analysis: Why ARP Fails
When the physical carrier is functional but ARP replies are missing, the failure generally falls into two categories: **Environmental Mismatches** or **Node Presence Issues**.

###### 1. Environmental & Configuration Mismatches
Even if both nodes are "on," they may be logically isolated or confused by the network configuration.

*   **Multicast/Channel Isolation:** If QEMU nodes use different multicast addresses or ports, they are on different virtual "wires." A broadcast sent on Port 1234 will never be heard by a listener on Port 5555.
*   **VLAN Tagging:** If Node A encapsulates its ARP request in a **VLAN Tag** (802.1Q) that Node B is not configured to strip, Node B will drop the frame as "garbage" because the header format is unexpected.
*   **MAC Address Duplication:** If both nodes share the same MAC address, the network driver may drop incoming packets, assuming it is hearing its own "echo" rather than a legitimate response from a peer.

###### 2. Node Presence & Security Dropping
The request reaches the wire, but the target cannot or will not respond.

*   **The Target is Absent:** The simplest explanation; no hardware with that IP address is connected to the segment.
*   **Firewall/Filtering:** The target node receives the ARP request, but a security policy (such as `arptables` or `nftables`) drops the packet before the kernel can generate a reply.
*   **Administrative Suppression (`NOARP`):** As seen in our experiment, the interface may be configured to ignore the ARP protocol entirely. This is common in specialized "Point-to-Point" or "Static" network environments.

##### 2. Request is not sent from host 
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

