## Exercise
> **Document Scope:** This document covers systematic, step-by-step diagnosis of Data Link Layer failures in a two-node QEMU Multicast lab. Every step produces a measurable result. A failure at any step identifies the exact component that is broken.

The **Data Link Layer (Layer 2)** is responsible for delivering data between two devices that are **directly connected on the same physical or virtual wire**. It does not deal with IP addresses or routing. Its only concern is: given a destination IP address, which **MAC address** on this wire does it belong to?

## The Ordered Phase Checklist

Execute phases in this exact order. Do not skip ahead. A failure at any phase means the root cause is within that phase — subsequent phases cannot succeed until the current one is resolved.

```
Phase 0: Physical Layer Healthy?           --> if NO: fix Layer 1 first (see Layer 1 doc)
             |
             v YES
Phase 1: ARP Request sent by Node A?      --> if NO: fix sender-side issue
             |
             v YES
Phase 2: ARP Request received by Node B?  --> if NO: fix wire/config issue
             |
             v YES
Phase 3: ARP Reply sent by Node B?        --> if NO: fix receiver-side ARP suppression
             |
             v YES
Phase 4: ARP Reply received by Node A?    --> if NO: fix return path
             |
             v YES
Phase 5: Neighbor Table populated?        --> if NO: fix kernel processing issue
             |
             v YES
         DATA LINK LAYER IS FUNCTIONAL
```

Without a MAC address, the kernel cannot build the Ethernet frame header. The IP packet cannot leave the machine.

```
APPLICATION
    |
    | (wants to send data to 192.168.1.2)
    v
IP LAYER
    |
    | (builds an IP packet, destination = 192.168.1.2)
    | (asks: "what MAC address is 192.168.1.2?")
    |
    +--- if MAC is KNOWN ----> builds Ethernet frame, sends it
    |
    +--- if MAC is UNKNOWN --> ARP process begins
                               if ARP fails --> IP packet is dropped
                               LAYER 2 FAILURE
```

**This is why ARP is the central focus of Data Link Layer troubleshooting.** ARP is not optional — it is the mandatory bridge between Layer 3 (IP) and Layer 2 (Ethernet).

`/proc/net/snmp` and `InReceives` count **IP packets** only. ARP frames carry EtherType `0x0806`, not `0x0800` (IPv4). The IP stack never sees ARP frames. This is a critical diagnostic distinction:

```
Frame arrives at eth0 driver
        |
        | (driver reads EtherType)
        |
        +--- EtherType = 0x0800 (IPv4) ----> handed to IP stack --> InReceives increments
        |
        +--- EtherType = 0x0806 (ARP)  ----> handed to ARP handler --> InReceives does NOT increment
        |
        +--- EtherType = 0x8100 (VLAN) ----> needs tag stripping first --> may be dropped
```
check procs [Refer](../../proc.md)
**Diagnostic Rule:** If `cat /proc/net/dev` shows received packets on `eth0` but `cat /proc/net/snmp` shows zero `InReceives`, the traffic on the wire is ARP — not IP. This is expected during address resolution.


## ARP failure
This is observed when Node A broadcasts a request but receives total silence from the network.

### Setup - Hardware not support ARP
By following these steps, a controlled failure is created to observe how the stack behaves when logical-to-physical mapping fails.
1.  **Preparation (Node B):** Disable ARP response logic: `ip link set dev eth0 arp off`.
2.  **Observation (Node A,B - Telnet Terminal):** Start the sniffer from `telnet` terminal: `tcpdump -i eth0 -e -n`.
    1. Start telnet `telnet localhost 4444` in a new terminal from host
    2. execute sniffer program: `tcpdump -i eth0 -e -n`
        - `-e`: By default, tcpdump only shows you Layer 3 (IP addresses) and above.  The `-e` flag forces it to show the Layer 2 (Link Layer) information.
        - `-n`: By default, tcpdump tries to be "helpful" by turning IP addresses and port numbers into names (e.g., converting 8.8.8.8 to google-public-dns-a.google.com). `-n` It disables "name resolution." It keeps 192.168.1.1 as 192.168.1.1.
3.  **Execution (Node A - Main Terminal):** 
    * Attempt to reach the peer by using ping command : `ping -c 2 192.168.1.2`.
    * While a standard ping uses ICMP (Layer 3), an arping sends ARP Requests (Layer 2). Many systems are configured to ignore `ICMP` "Echo Requests" (standard pings) for security. However, no host can communicate on an Ethernet network without responding to ARP. If ping fails but arping succeeds, the host is alive but hiding behind a firewall. command: `arping -I eth0 192.168.1.2`

### Setup - Firewall
By following these steps, a controlled failure is created to observe how the stack behaves when logical-to-physical mapping fails.

1. **Preparation (Node B):**
    ```
    nft add table arp filter
    nft add chain arp filter input { type filter hook input priority 0 \; policy accept \; }
    nft add rule arp filter input iifname "eth0" drop
    ```
2. **Observation (Node A,B - Telnet Terminal):** Start the sniffer from `telnet` terminal: `tcpdump -i eth0 -e -n`.
    **Execution (Node A - Main Terminal):** 
    * Attempt to reach the peer by using ping command : `ping -c 2 192.168.1.2`.

## Phase 1: Confirm ARP Request Is Sent (Node A — Sender)

This phase determines whether Node A's kernel is building and transmitting ARP frames at all.

### Step 1.1 — Open the Sniffer on Node A (Telnet Terminal)

Connect to Node A's telnet terminal from the host:

```bash
telnet localhost 4444
```

Start the packet capture on the experimental interface:

```bash
tcpdump -i eth0 -e -n
```

### Step 1.2 — Trigger ARP From Node A (Main Terminal)

```bash
ping -c 2 192.168.1.2
```

### Step 1.3 — Read the Sniffer Output

**Expected output (ARP request visible):**

```
23:15:32.054505 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42:
    Request who-has 192.168.1.2 tell 192.168.1.1
```

**Reading the output fields:**

```
52:54:00:12:34:01           Source MAC (Node A's MAC)
ff:ff:ff:ff:ff:ff           Destination MAC (broadcast — sent to everyone)
ethertype ARP (0x0806)      EtherType confirms this is ARP, not IP
Request who-has 192.168.1.2 The question: "Which device has this IP?"
tell 192.168.1.1            Node A is asking, using its own IP as the return address
```

**If this line appears:** ARP request is being generated and transmitted. Proceed to Phase 2.

**If NO output appears at all:**

The kernel is not generating ARP. Investigate:

**Check 1 — Routing table**

```bash
ip route show
```

Expected entry:

```
192.168.1.0/24 dev eth0 scope link src 192.168.1.1
```

This line means: "For destination IPs in range 192.168.1.x, use eth0 and resolve the MAC directly via ARP." If this entry is missing, the kernel does not know which interface to use and will not generate ARP.

**If the entry is missing:**

```bash
ip addr add 192.168.1.1/24 dev eth0
```

Adding an IP address automatically creates this routing entry.

**Check 2 — Interface has an IP assigned**

```bash
ip addr show eth0
```

The `inet 192.168.1.1/24` line must be present. Without an IP, the kernel cannot form an ARP request (it has no "sender IP" field to populate in the ARP frame).


**Check 3 — Egress firewall**

```bash
nft list ruleset
```

if return empty no firewall, else looks for rules with 
Look for any `DROP` or `REJECT` rules. If found, they intercept the outgoing ARP frame before it reaches the driver.

```bash
# Verify by checking outgoing packet counters
cat /proc/net/dev
```

If the `Transmit packets` column on `eth0` does not increment when `ping` is run, the frame is being dropped before leaving the machine.

##  Phase 2: Confirm ARP Request Arrives (Node B — Receiver)

This phase confirms the ARP request crossed the virtual wire and reached Node B's hardware interface.

### Step 2.1 — Open the Sniffer on Node B (Telnet Terminal)

From the host, open a new terminal:

```bash
telnet localhost 4445
```

Start the capture:

```bash
tcpdump -i eth0 -e -n
```

### Step 2.2 — Check Hardware Reception

While Node A is pinging, confirm the ARP request is visible in Node B's tcpdump:

**Expected output on Node B:**

```
23:15:32.054505 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42:
    Request who-has 192.168.1.2 tell 192.168.1.1
```

**If this line appears:** The virtual wire is functional. The ARP request reached Node B's physical layer. Proceed to Phase 3.

**If nothing appears on Node B's tcpdump:**

The frame never crossed the virtual wire. Investigate:

**Check 1 — Multicast address and port match**

Compare the QEMU launch commands of both nodes:

```
Node A: -netdev socket,id=n1,mcast=230.0.0.1:1234
Node B: -netdev socket,id=n1,mcast=230.0.0.1:1234
                                   ^^^^^^^^^^^^^^^^^^^
                                   Must be character-for-character identical
```

Even a one-digit difference (port `1234` vs `1235`) places the nodes on entirely different virtual wires. There is no error message — packets simply disappear.

**Check 2 — Confirm Node B's eth0 is up**

```bash
ip link show eth0
```

`LOWER_UP` must be present. If not, the interface is not listening on the wire.

**Check 3 — Confirm interface packet counters on Node B**

```bash
cat /proc/net/dev
```

Observe the `Receive packets` column for `eth0`. Trigger several pings from Node A. If the counter does not increment, the virtual wire itself is not delivering frames.

## Phase 3: Confirm ARP Reply Is Sent (Node B — Responder)

The ARP request arrived at Node B's hardware (Phase 2 passed). This phase confirms that Node B's kernel processes the request and transmits a reply.

### Step 3.1 — Check for ARP Reply in Node B's tcpdump

After confirming the request arrives, the sniffer on Node B should show two lines — the incoming request and an outgoing reply:

**Expected output on Node B (both lines):**

```
23:15:32.054505 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42:
    Request who-has 192.168.1.2 tell 192.168.1.1        <-- incoming from Node A

23:15:32.054600 52:54:00:12:34:02 > 52:54:00:12:34:01, ethertype ARP (0x0806), length 42:
    Reply 192.168.1.2 is-at 52:54:00:12:34:02           <-- outgoing from Node B
```

**Reading the reply line:**

```
52:54:00:12:34:02           Node B's MAC (the responder)
52:54:00:12:34:01           Node A's MAC (unicast reply — only the asker needs this)
ethertype ARP (0x0806)      Still ARP protocol
Reply 192.168.1.2 is-at ... The answer: "I have this IP, and my MAC is this"
```

**If the request appears but NO reply appears:**

Node B's kernel received the frame but suppressed the ARP reply. Three causes, checked in order:

#### Failure A: NOARP Flag Set

**Check:**

```bash
ip link show eth0
```

**Symptom:**

```
2: eth0: <BROADCAST,MULTICAST,NOARP,UP,LOWER_UP>
                              ^^^^^
                              NOARP = this interface will not process ARP at all
```

**Why this happens:** `ip link set dev eth0 arp off` was executed. The flag instructs the kernel to completely ignore the ARP protocol on this interface — it will receive the frame but discard it without generating any reply.

**Fix:**

```bash
ip link set dev eth0 arp on
```

**Verify:** Re-run ping from Node A. The `NOARP` flag must disappear from `ip link show`.

#### Failure B: ARP Filtered by arptables or nftables

**Check:**
```bash
nft list ruleset 2>/dev/null
```

Look for any `DROP` rule targeting ARP traffic. If found, it intercepts the ARP frame before the kernel generates a reply.

**Diagnostic distinction:** With `NOARP`, the interface flag is visible in `ip link`. With a firewall rule, `ip link` shows no `NOARP` flag — the interface looks normal, but the reply is silently discarded by Netfilter.

**Fix:** Remove the offending rule. Example for nftables:

```bash
nft flush ruleset
```

#### Failure C: IP Address Missing or Mismatched on Node B

**Check:**

```bash
ip addr show eth0
```

The `inet 192.168.1.2/24` line must be present. The kernel only replies to ARP requests for IPs that are **assigned to that interface**. If Node B's IP was never assigned, or was assigned to the wrong interface, the kernel sees the ARP request for `192.168.1.2` and correctly stays silent because it does not own that address.

**Fix:**

```bash
ip addr add 192.168.1.2/24 dev eth0
```

## Phase 4: Confirm ARP Reply Arrives (Node A — Final Check)

The ARP reply was sent by Node B (Phase 3 passed). This phase confirms the reply crossed the wire back to Node A.

### Step 4.1 — Check for ARP Reply in Node A's tcpdump

The sniffer running on Node A's telnet terminal should show both the outgoing request and the incoming reply:

**Expected output on Node A:**

```
23:15:32.054505 52:54:00:12:34:01 > ff:ff:ff:ff:ff:ff, ethertype ARP (0x0806), length 42:
    Request who-has 192.168.1.2 tell 192.168.1.1        <-- outgoing (Node A sent this)

23:15:32.054650 52:54:00:12:34:02 > 52:54:00:12:34:01, ethertype ARP (0x0806), length 42:
    Reply 192.168.1.2 is-at 52:54:00:12:34:02           <-- incoming (Node B's reply)
```

**If the request appears on Node A but the reply never appears:**

The reply left Node B but did not reach Node A. This is a wire-return path problem.

**Check 1 — Same as Phase 2 virtual wire checks:** Confirm multicast address/port match. In a multicast hub topology, the reply travels the same wire in reverse. If that wire is functional in one direction, it is functional in both — re-examine the multicast configuration.

**Check 2 — MAC address duplication**

If Node A and Node B were accidentally assigned the same MAC address, Node A's driver will receive the reply and discard it — the NIC identifies the frame as its own "echo" coming back.

```bash
# On Node A
ip link show eth0 | grep ether

# On Node B
ip link show eth0 | grep ether
```

The two `link/ether` values must be different. If they are identical, one node must be relaunched with a corrected MAC address in the QEMU command.

## Phase 5: Confirm Neighbor Table Population

If Phase 4 confirms the ARP reply arrived at Node A, the kernel must store the MAC-to-IP mapping in the **Neighbor Table** (also called the ARP Cache).

### Step 5.1 — Check the Neighbor Table

```bash
ip neigh show
```

**Expected output after successful ARP:**

```
192.168.1.2 dev eth0 lladdr 52:54:00:12:34:02 REACHABLE
```

**Reading each field:**

| Field | Meaning |
| :--- | :--- |
| `192.168.1.2` | The IP address that was resolved |
| `dev eth0` | The interface through which this neighbor is reachable |
| `lladdr 52:54:00:12:34:02` | The MAC address discovered by ARP (lladdr = Link Layer Address) |
| `REACHABLE` | The kernel has confirmed this mapping is currently valid |

### Static ARP (Emergency Workaround)

When the ARP protocol itself is blocked or broken but IP connectivity still needs to be tested, a static ARP entry bypasses the entire dynamic ARP process:

```bash
# On Node A: manually tell the kernel where Node B is, without ARP
ip neigh add 192.168.1.2 lladdr 52:54:00:12:34:02 dev eth0 nud permanent

# Then test IP connectivity
ping 192.168.1.2
```

**Purpose of this technique:** If `ping` succeeds after a static ARP entry is added, it confirms the failure is isolated to the ARP protocol itself — IP, routing, and the wire are all functional.

### Stale cleanup
If a user fixes a configuration (like turning ARP back on) but previously had a failed attempt, the kernel might back off or hold onto a FAILED state for several seconds.

```bash
ip neigh flush dev eth0
```
This ensures the next ping triggers a fresh ARP cycle rather than relying on cached (and potentially broken) data.
