# Complete Reference Documentation: Virtual Local Area Networks (VLANs)

> **Document Purpose:** These notes are written for an absolute beginner with zero prior networking knowledge. Every concept is explained from the ground up, including the logic behind it. A reader returning after one year must fully understand all material without consulting any external resource.


## 1. Foundation: What Is a Network?

### 1.1 The Basic Concept

A **network** is a collection of devices (computers, printers, phones) connected together so they can send and receive data between each other.

The physical device that connects multiple devices in a local area is called a **Switch**. A switch receives data from one device and forwards it to the correct destination device.

### 1.2 What Is a Broadcast Domain?

When a device wants to find another device on the same network (for example, to find its IP-to-MAC address mapping), it sends a **broadcast packet**. A broadcast is a message addressed to every single device on the network — it says "everyone, please receive this."

A **Broadcast Domain** is the set of all devices that will receive that broadcast message.

**The problem with large broadcast domains:**

```
Switch with 200 devices connected
|
One device sends a broadcast
|
All 200 devices receive it, interrupt their work, and process it
|
This happens hundreds of times per day
|
Result: Wasted bandwidth, wasted CPU cycles on every device
```

The larger the broadcast domain, the worse this problem becomes.

## 2. The Problem VLANs Solve

A standard network without VLANs is called a **flat network**. In a flat network:

- Every device connected to the same switch is in the same broadcast domain.
- Every device can potentially communicate with every other device.
- There is no logical separation between departments or functions.

### 2.1 Three Core Problems of a Flat Network

**Problem 1 — Broadcast Noise**

Every broadcast reaches every device, even irrelevant ones. A broadcast from a device in the Accounting department travels to every device in the Engineering department, the Guest Wi-Fi, the Security cameras — even though none of them care about it.

**Problem 2 — Security Risk**

Any device on the network can attempt to communicate with any other device. There is no built-in separation between sensitive data (Accounting records) and low-trust areas (Guest Wi-Fi). If a guest device is compromised, it has direct network-level access to everything else.

**Problem 3 — Hardware Cost**

Without VLANs, the only way to create separate, isolated network segments is to buy a separate physical switch for each segment. For an organization with 10 departments, this means 10 physical switches.

### 2.2 How VLANs Solve These Problems

| Problem | VLAN Solution |
| :--- | :--- |
| Broadcast Noise | Broadcasts are restricted to only the ports in the same VLAN |
| Security Risk | Devices in different VLANs cannot see each other's traffic at Layer 2 |
| Hardware Cost | One 48-port physical switch can act as 10+ independent virtual switches |

## 3. What Is a VLAN?

A **Virtual Local Area Network (VLAN)** is a **logical** broadcast domain created by software configuration on a physical switch.

The word **"virtual"** means it does not require separate physical hardware. The division exists in the switch's software and configuration, not in the cables or ports.

The word **"logical"** means it is based on rules and assignment, not on physical location. A device's VLAN membership is determined by which port it is plugged into (or by its MAC address), not by where it physically sits in a building.

### 3.1 A Visual Comparison

**Flat Network (No VLANs) — One physical switch, one broadcast domain:**

```
+--------------------------------------------------+
|              PHYSICAL SWITCH                     |
|                                                  |
| [PC-1] [PC-2] [PC-3] [PC-4] [PC-5] [PC-6]        |
|                                                  |
|   ALL devices are in ONE broadcast domain        |
|   A broadcast from PC-1 reaches ALL 6 devices    |
+--------------------------------------------------+

Network: 192.168.10.0/24
PC-1 : 192.168.10.2     PC-2 : 192.168.10.3     PC-3 : 192.168.10.4
PC-4 : 192.168.10.5     PC-5 : 192.168.10.6     PC-7 : 192.168.10.7
```


**With VLANs — One physical switch, three separate logical broadcast domains:**

```
+--------------------------------------------------+
|              PHYSICAL SWITCH                     |
|                                                  |
| +----------------+  +-----------+  +-----------+ |
| |    VLAN 10     |  |  VLAN 20  |  |  VLAN 30  | |
| | (Accounting)   |  | (Eng.)    |  | (Guests)  | |
| | [PC-1] [PC-2]  |  | [PC-3]    |  | [PC-5]    | |
| |                |  | [PC-4]    |  | [PC-6]    | |
| +----------------+  +-----------+  +-----------+ |
|                                                  |
|  A broadcast from PC-1 ONLY reaches PC-2         |
+--------------------------------------------------+

VLAN 10  (Accounting) -->  192.168.10.0/24   (PC-1: .10.2,  PC-2: .10.3)
VLAN 20  (Eng.)       -->  192.168.20.0/24   (PC-3: .20.2,  PC-4: .20.3)
VLAN 30  (Guests)     -->  192.168.30.0/24   (PC-5: .30.2,  PC-6: .30.3)

```

PC-1 and PC-5 are plugged into the same physical switch, but they are completely isolated from each other at Layer 2.

**Each VLAN is assigned its own separate subnet. This is mandatory, not optional.**

#### Where the Router Fits In
The router has one logical interface per VLAN. These are called sub-interfaces or SVI (Switched Virtual Interfaces).
```
+-------------------------------------------------+
|              PHYSICAL SWITCH                    |
|                                                 |
| +------------+   +-----------+   +------------+ |
| |  VLAN 10   |   |  VLAN 20  |   |  VLAN 30   | |
| | 192.168.10 |   | 192.168.20|   | 192.168.30 | |
| | [PC-1]     |   | [PC-3]    |   | [PC-5]     | |
| | [PC-2]     |   | [PC-4]    |   | [PC-6]     | |
| +-----+------+   +-----+-----+   +------+-----+ |
|       |                |                |       |
|       +----------------+----------------+       |
|                        |  (Trunk Port)          |
+------------------------+------------------------+
                         |
                    +----+----+
                    | ROUTER  |
                    |         |
                    | eth0.10 | <-- Gateway for VLAN 10 = 192.168.10.1
                    | eth0.20 | <-- Gateway for VLAN 20 = 192.168.20.1
                    | eth0.30 | <-- Gateway for VLAN 30 = 192.168.30.1
                    +---------+
```
Each sub-interface acts as the default gateway for that VLAN's subnet.


## 4. The OSI Model (Relevant Layers Only)

Understanding where VLANs operate requires a basic understanding of the **OSI Model**. The OSI (Open Systems Interconnection) model is a conceptual framework that divides networking functions into 7 layers. Each layer has a specific job.

For VLAN study, only two layers are essential:

```
+-----+----------------------+----------------------------------------------+
| No. | Layer Name           | Job (Simple Description)                     |
+-----+----------------------+----------------------------------------------+
|  3  | Network Layer        | Moves data BETWEEN different networks.       |
|     |                      | Uses IP Addresses. (Routers work here)       |
+-----+----------------------+----------------------------------------------+
|  2  | Data Link Layer      | Moves data WITHIN one network segment.       |
|     |                      | Uses MAC Addresses. (Switches work here)     |
+-----+----------------------+----------------------------------------------+
```

**Key Rule:** VLANs are a **Layer 2** technology. They operate using MAC addresses and Ethernet frames. To move data between two different VLANs, a **Layer 3** device (a router) is required.

## 5. The Ethernet Frame (The Basic Unit of Data)

Before understanding how VLANs tag data, the structure of a standard Ethernet frame must be understood first. A **frame** is the structured container that holds data as it travels across a network.

### 5.1 Standard Ethernet Frame Structure (Untagged)

```
+-------------+-------------+-----------+--------------------+---------+-------+
|  Dest MAC   |   Src MAC   | Type/Len  |   Data (Payload)   | Padding |  FCS  |
|  (6 Bytes)  |  (6 Bytes)  | (2 Bytes) |   (46-1500 Bytes)  |  (Var)  | (4 B) |
+-------------+-------------+-----------+--------------------+---------+-------+
     ^               ^            ^               ^               ^         ^
     |               |            |               |               |         |
     |               |            |               |               |         +-- Frame Check Sequence
     |               |            |               |               +-- Zero-padding to reach minimum size
     |               |            |               +-- The actual data being sent
     |               |            +-- Identifies the protocol inside the payload (e.g., 0x0800 = IPv4)
     |               +-- MAC address of the device SENDING the frame
     +-- MAC address of the device the frame is GOING TO
```

### 5.2 Field-by-Field Explanation


**EtherType / Length (2 Bytes)**
Identifies what type of data is inside the payload. For example:
- `0x0800` means the payload contains an IPv4 packet.
- `0x0806` means the payload contains an ARP packet.
- `0x8100` means this frame has a VLAN tag (explained in Section 6).

**Data / Payload (46–1500 Bytes)**
The actual content being transported — the IP packet, the ARP request, etc. The minimum is 46 bytes and the maximum is 1500 bytes.

**Padding (Variable)**
If the payload data is smaller than 46 bytes, zero-value bytes are added to reach the minimum. This is required by the Ethernet specification. (Full explanation in Section 12.)


### 5.3 Frame Size Summary

| Measurement | Value |
| :--- | :--- |
| Minimum frame size | 64 bytes |
| Maximum frame size (standard) | 1518 bytes |
| Header size (untagged) | 14 bytes (6+6+2) |
| Minimum payload | 46 bytes |
| FCS | 4 bytes |

## 6. The IEEE 802.1Q VLAN Tag (How VLANs Are Marked)

When a frame travels across the network between two switches, the switches need a way to know which VLAN that frame belongs to. The standard mechanism for this is the **IEEE 802.1Q tag**.

**IEEE** stands for Institute of Electrical and Electronics Engineers — the organization that defines networking standards. **802.1Q** is the specific standard number for VLAN tagging.

### 6.1 What the Tag Is

The 802.1Q tag is a **4-byte insertion** placed inside an Ethernet frame. It is not a separate packet — it is a modification of an existing frame.

### 6.2 Where the Tag Is Inserted

The tag is inserted between the **Source MAC Address** and the **EtherType/Length** field.

#### Who Attaches and Removes VLAN Tags
End devices — laptops, printers, desktop PCs — have no knowledge of VLAN tags. They send and receive standard Ethernet frames only. The switch is entirely responsible for tag management at both ends. When an untagged frame arrives from a PC on an Access Port, the switch reads that port's configured VLAN ID and attaches the 802.1Q tag internally before processing the frame. When that same frame needs to be delivered to another PC on another Access Port, the switch strips the tag completely before sending it out — the destination PC receives a clean, standard Ethernet frame with no trace of any VLAN information. The PC never saw the tag on the way in, and it never sees it on the way out. From every end device's perspective, the network is a simple flat Ethernet — the entire VLAN system is an invisible operation happening inside the switch.


**Before tagging (standard frame):**

```
+-------------+-------------+-----------+------------------+-------+
|  Dest MAC   |   Src MAC   | Type/Len  |  Data (Payload)  |  FCS  |
|  (6 Bytes)  |  (6 Bytes)  | (2 Bytes) |  (46-1500 B)     | (4 B) |
+-------------+-------------+-----------+------------------+-------+
                            ^
                            |
                      Tag inserted HERE
```

**After tagging (802.1Q frame):**

```
+-------------+-------------+============+-----------+------------------+-------+
|  Dest MAC   |   Src MAC   | 802.1Q TAG | Type/Len  |  Data (Payload)  |  FCS  |
|  (6 Bytes)  |  (6 Bytes)  | (4 Bytes)  | (2 Bytes) |  (42-1500 B)     | (4 B) |
+-------------+-------------+============+-----------+------------------+-------+
```


Switch received maximum size of the frame, will it truncated?

```
PC sends a maximum-size untagged frame:

+----------+----------+-----------+------------------+-------+
| Dest MAC |  Src MAC | EtherType |  Payload (1500B) |  FCS  |
|  6 Bytes |  6 Bytes |  2 Bytes  |   1500 Bytes     | 4 B   |
+----------+----------+-----------+------------------+-------+
Total = 6 + 6 + 2 + 1500 + 4 = 1518 bytes  <-- Standard Maximum
```

Now the switch needs to forward this out a Trunk Port. It inserts the 4-byte VLAN tag:

```
+----------+----------+============+-----------+------------------+-------+
| Dest MAC |  Src MAC | VLAN TAG   | EtherType |  Payload (1500B) |  FCS  |
|  6 Bytes |  6 Bytes |  4 Bytes   |  2 Bytes  |   1500 Bytes     | 4 B   |
+----------+----------+============+-----------+------------------+-------+
Total = 6 + 6 + 4 + 2 + 1500 + 4 = 1522 bytes  <-- EXCEEDS standard by 4 bytes
```

**The payload was NOT changed. The data was NOT truncated. The frame simply grew by 4 bytes.**

### This Frame Has a Name: "Baby Giant"

```
Normal Frame:        up to 1518 bytes   (standard, no tag)
Baby Giant Frame:    up to 1522 bytes   (standard + 802.1Q tag)
Jumbo Frame:         up to 9000+ bytes  (special high-performance networks)
```

The 1522-byte frame is called a **Baby Giant** — it is only slightly over the old limit, specifically because of the VLAN tag.

### What Actually Happens: Two Scenarios

#### Scenario A — Modern Equipment (Expected Case)

Modern switches, routers, and NICs are built with 802.1Q awareness. They accept **1522 bytes as a valid maximum** for tagged frames.

```
Switch sends 1522-byte tagged frame
        |
        v
Next switch receives it
        |
        | Checks: Is TPID = 0x8100? YES.
        | Accepts up to 1522 bytes as valid.
        |
        v
Frame processed normally. No problem.
```

The 802.1Q standard explicitly defines 1522 bytes as the new maximum when tagging is involved. Any device that claims 802.1Q support must accept this.

#### Scenario B — Old/Misconfigured Equipment (The Real Problem)

An old switch or a misconfigured device has its maximum frame size hardcoded at 1518 bytes.

```
Switch sends 1522-byte tagged frame
        |
        v
Old switch receives it
        |
        | Checks: Is this frame larger than 1518 bytes? YES (it is 1522).
        | Classifies it as: "Giant Frame" (error condition)
        |
        v
Frame is DROPPED SILENTLY.
```

**Silent drop** is the dangerous part. No error message is sent back. From the PC's perspective, the data just disappeared. This causes:

```
Symptoms visible to the user:
- Large file transfers fail randomly
- Web pages load partially then hang
- Small requests (like pings) work fine  <-- because small frames never hit 1518B
- Video streams buffer endlessly
- SSH sessions drop mid-transfer
```

Small frames work because they are well under 1518 bytes even after tagging. Only maximum-size frames trigger the problem. This makes it extremely difficult to diagnose.

### Why the Payload Cannot Simply Be Trimmed

A logical question: why not just cut 4 bytes from the payload to keep the total at 1518?

```
WRONG approach:
  Take 1500-byte payload, cut it to 1496 bytes, insert 4-byte tag --> 1518 bytes total
```

The switch **cannot do this**. The payload is not raw bytes the switch invented — it is an IP packet from the PC. Cutting 4 bytes from an IP packet corrupts it. The IP header contains a checksum and a length field that would now be wrong. The receiving device would discard it as malformed.

The switch's job is to **forward data, not modify it**.

### The Correct Solutions

#### Solution 1 — Reduce MTU on End Devices (The Proper Fix)

The MTU (Maximum Transmission Unit) is the largest IP payload a device is allowed to send in one frame. The default is **1500 bytes**.

If end devices are configured to use MTU **1496 bytes** instead:

```
PC sends frame with 1496-byte payload (MTU reduced by 4):

Untagged total = 6 + 6 + 2 + 1496 + 4 = 1514 bytes

Switch adds 4-byte VLAN tag:

Tagged total  = 6 + 6 + 4 + 2 + 1496 + 4 = 1518 bytes  <-- fits perfectly
```

The frame never exceeds 1518 bytes even after tagging.

**Downside:** Slightly reduced throughput because each frame carries 4 fewer bytes of useful data.

#### Solution 2 — Enable Baby Giant Support on All Switches

Configure every switch in the path to accept frames up to **1522 bytes** as valid.

```
On a switch (example):
system mtu 1522
```

This is the preferred solution in modern networks. No end device configuration needed.

#### Solution 3 — Replace Old Equipment

If a device cannot be configured to accept 1522-byte frames, it does not support 802.1Q properly. It must be replaced.

#### Complete Picture

```
                  1500B payload from PC
                          |
                    1518B untagged frame
                          |
                   Arrives at switch
                          |
              Switch adds 4-byte VLAN tag
                          |
                    1522B tagged frame
                          |
           +--------------+--------------+
           |                             |
    Modern switch                  Old/misconfigured
    (802.1Q aware)                    switch
           |                             |
    Accepts 1522B                  Sees >1518B
    Forwards normally              DROPS frame silently
```

#### One-Line Summary

> When a 1500-byte payload frame gets VLAN-tagged, it becomes 1522 bytes — a "Baby Giant." Modern equipment accepts this by design. Old equipment silently drops it. The fix is either reducing the sender's MTU by 4 bytes or configuring all switches to accept 1522-byte frames.

**Why is the tag placed in this specific position?**

The Destination MAC and Source MAC fields must remain at the very front of the frame because every network device reads them first to make forwarding decisions. Placing the tag after the Source MAC ensures the fundamental addressing information is never disturbed.

## 7. The 4-Byte Tag: Field-by-Field Breakdown

The 4-byte (32-bit) VLAN tag contains four distinct fields. Each bit of these 32 bits has a specific meaning.

### 7.1 Visual Layout of the 32 Bits

```
Bit:  0               1               2               3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8   9   0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+-+-+-+-+-+-+-+-+-+-+-+
      |               TPID            |  PCP  |DEI|          VID        |
      |            (16 bits)          |(3bits)|(1)|      (12 bits)      |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---+---+-+-+-+-+-+-+-+-+-+-+-+
```

### 7.2 Field 1: TPID — Tag Protocol Identifier (16 bits)

**Value:** Always `0x8100` (hexadecimal).

**Purpose:** This field acts as a signal to any device reading the frame. When a switch or network parser reads the frame and finds `0x8100` in the position where EtherType normally sits, it knows: "This is not a standard Ethernet frame. This frame has a VLAN tag. The next 2 bytes are not the start of the payload — they are VLAN tag information."

**Why this matters for software:** When writing a network protocol parser in code, the parser must check this field first. If the value is `0x8100`, the parser must skip 4 additional bytes before reading the actual EtherType and payload. If this check is skipped, the parser will misread the frame entirely.

### 7.3 Field 2: PCP — Priority Code Point (3 bits)

**Value range:** 0 to 7 (because 3 bits can represent 2³ = 8 values).

**Purpose:** Marks the frame with a priority level for **Quality of Service (QoS)**.

**Why this exists:** Not all network traffic is equally urgent. A video call requires data to arrive with low delay. A file download in the background can tolerate some delay. Without a priority system, a switch treats all frames equally and processes them in the order they arrive. During congestion, this causes video calls to freeze even though a bulk file transfer could wait.

PCP solves this by letting the sender label the urgency of each frame. The switch processes high-priority frames first.

```
PCP Value |  Priority Level  | Typical Use
----------|------------------|--------------------------
    7     |  Highest         | Network control traffic
    6     |  High            | Internetwork control
    5     |  Medium-High     | Voice (VoIP)
    4     |  Medium          | Video
    3     |  Normal-High     | Critical data
    2     |  Low             | Standard data
    1     |  Lower           | Background
    0     |  Lowest          | Best-effort (default)
```

### 7.4 Field 3: DEI — Drop Eligible Indicator (1 bit)

**Value:** 0 or 1 (one bit).

**Purpose:** When this bit is set to `1`, it signals to the switch that this frame is allowed to be dropped first if the network is congested.

**Why this exists:** When a switch's internal buffer fills up (too much traffic arriving faster than it can be forwarded), some frames must be dropped. The DEI bit allows the network operator to pre-mark certain lower-importance frames as acceptable to drop, protecting higher-importance frames.

> **Historical Note:** This field was previously named **CFI (Canonical Format Indicator)** in older standards. Modern documentation uses DEI.

### 7.5 Field 4: VID — VLAN Identifier (12 bits)

**Value range:** 0 to 4095 (because 12 bits can represent 2¹² = 4096 values).

**Purpose:** This is the actual VLAN ID number. This single field is the core of VLAN tagging — it tells any switch exactly which VLAN this frame belongs to.

**Reserved Values:**

| VID Value | Meaning |
| :--- | :--- |
| 0 | Reserved — used for priority tagging only, no VLAN ID |
| 1 | Default VLAN (Native VLAN on most switches) |
| 2–4093 | **Usable VLAN IDs** — assignable by the administrator |
| 4094 | Usable (in practice, sometimes reserved by vendors) |
| 4095 | Reserved — not for use |

**Practical Result:** The 12-bit VID allows **4,094 usable unique VLANs** on a single network. This is more than enough for any enterprise network.

## 8. Port Types: Access Ports and Trunk Ports

A **port** is a physical connection point on a switch — the socket where a cable is plugged in. Switches have two fundamentally different port configurations, and each serves a specific purpose.

### 8.1 Access Ports

**Definition:** A port configured to belong to exactly **one** VLAN.

**Typical connected devices:** End-user devices — desktop computers, laptops, printers, IP cameras, VoIP phones.

**Core rule:** The device connected to an Access Port has no knowledge of VLANs. It sends and receives standard, untagged Ethernet frames.

#### The Access Port Workflow (Step by Step)

```
STEP 1: PC sends a standard, untagged frame into the switch port
        
        [PC] ----untagged frame----> [Switch Port: Access, PVID=20]

STEP 2: The switch receives the untagged frame.
        It reads the port's assigned PVID (Port VLAN ID = 20).
        It internally marks the frame as belonging to VLAN 20.
        (The tag is added inside the switch's memory — not written to the physical wire yet)

STEP 3: The switch checks: which other ports belong to VLAN 20?
        The frame is only allowed to travel to those ports internally.

STEP 4: The frame reaches the destination port (another Access Port, PVID=20).
        Before the frame leaves that port onto the wire, the switch STRIPS the VLAN tag completely.

STEP 5: The destination PC receives a completely standard, untagged Ethernet frame.
        The destination PC has no idea that VLANs were involved.
```

**Why the tag is stripped on egress:** Standard consumer NICs and operating systems expect a standard 14-byte Ethernet header. If a tagged frame arrives at a standard Windows or Linux machine, the OS reads the TPID (`0x8100`) where it expects the EtherType, does not recognize it, and discards the frame as malformed.

### 8.2 Trunk Ports

**Definition:** A port configured to carry traffic for **multiple VLANs** simultaneously over a single physical cable.

**Typical connected devices:** Other switches, routers, firewalls, and virtualization servers (like VMware ESXi hosts).

**Core rule:** Tags are preserved. When a tagged frame travels out of a Trunk Port, the VLAN tag is kept intact. The receiving device uses the VID to know which VLAN the frame belongs to.

**Why Trunk Ports exist:**

```
Building Floor 1: Switch A (has VLAN 10, 20, 30)
Building Floor 2: Switch B (has VLAN 10, 20, 30)

|
| Without Trunk Ports:
| Switch A needs 3 cables to Switch B — one cable per VLAN.
| For 50 VLANs: 50 cables. Physically impractical.
|
| With a Trunk Port:
| One single cable between Switch A and Switch B.
| All VLAN traffic travels over it, distinguished by the 802.1Q tag.
|
```

#### The Trunk Port Workflow (Step by Step)

```
STEP 1: PC on VLAN 20 (Switch A) sends an untagged frame to its access port.
        Switch A tags the frame internally: VID = 20.

STEP 2: The frame needs to reach a device on VLAN 20 on Switch B.
        Switch A forwards the frame out its Trunk Port to Switch B.
        The 802.1Q tag (VID=20) is kept on the frame.

STEP 3: Switch B receives the tagged frame on its Trunk Port.
        It reads the VID: 20. It knows this frame belongs to VLAN 20.

STEP 4: Switch B forwards the frame to the correct Access Port (PVID=20).
        Before leaving the Access Port, Switch B strips the tag.

STEP 5: The destination PC receives a clean, untagged standard frame.
```

### 8.3 Comparison Table

```
+-------------------+---------------------+----------------------------+
| Feature           | Access Port         | Trunk Port                 |
+-------------------+---------------------+----------------------------+
| Connected Device  | PC, Printer, Phone  | Switch, Router, VM Host    |
| VLAN Limit        | Exactly one VLAN    | Multiple VLANs             |
| Tag on Egress     | NO — tags stripped  | YES — tags preserved       |
| Native VLAN       | Not applicable      | One VLAN sent untagged     |
| Device awareness  | No VLAN awareness   | Must understand 802.1Q     |
+-------------------+---------------------+----------------------------+
```

## 9. VLAN Communication Rules

### 9.1 Rule: Devices in the Same VLAN Can Communicate Freely

Two devices assigned to VLAN 10 on the same switch (or across multiple switches connected by a trunk) can send frames to each other directly through the switch. The switch handles this at Layer 2 — no router is needed.

### 9.2 Rule: Devices in Different VLANs Cannot Communicate by Default

A device in **VLAN 10** cannot send data to a device in **VLAN 20** using Layer 2 alone. The switch treats them as completely separate networks.

**Why:** VLANs create separate broadcast domains. From the switch's perspective, VLAN 10 and VLAN 20 are as isolated as if they were on two completely different physical switches with no cable between them.

### 9.3 Rule: A Layer 3 Device Is Required for Inter-VLAN Communication

For a device in VLAN 10 to communicate with a device in VLAN 20, the traffic must be **routed** — it must travel up to Layer 3, be processed by a router or Layer 3 switch, and then be sent back down into the destination VLAN.

This is called **Inter-VLAN Routing**.

```
VLAN 10                    Layer 3 Device                 VLAN 20
Device A                   (Router or L3 Switch)          Device B
[IP: 192.168.10.5]  --->   [Performs routing]  --->      [IP: 192.168.20.8]
                           [Applies firewall rules]
                           [Checks ACLs]
```

**Why this design is intentional:** Requiring traffic to pass through a Layer 3 device means a firewall or Access Control List (ACL) can be placed on that device to inspect and filter inter-VLAN traffic. This enforces security boundaries between departments.

### 9.4 Rule: Tagged and Untagged Devices Communicate Through the Switch as Translator

A standard device (sending untagged frames) and a VLAN-aware device (sending tagged frames) can coexist on the same network because the switch handles the translation:

- Untagged frame arrives on an Access Port → Switch adds the tag internally.
- Tagged frame needs to leave an Access Port → Switch strips the tag before delivery.
- The end device never needs to understand 802.1Q.

## 10. How a Switch Assigns VLANs (Decision Logic)

When a frame arrives at a switch port, the switch must determine which VLAN the frame belongs to. There are three methods:

### Method 1: Static / Port-Based Assignment

The network administrator manually assigns each physical port to a VLAN.

```
Port 1  --> VLAN 10  (Accounting PC always plugged here)
Port 2  --> VLAN 10  (Another Accounting PC)
Port 3  --> VLAN 20  (Engineering PC)
Port 4  --> VLAN 30  (Guest Wi-Fi access point)
```

Any device plugged into Port 3 is automatically in VLAN 20, regardless of what the device is.

**Advantage:** Simple, predictable, and easy to audit.  
**Disadvantage:** If a user moves their laptop to a different port, they may land in the wrong VLAN.

### Method 2: Dynamic / MAC-Based Assignment

The switch maintains a database that maps specific MAC addresses to specific VLANs.

```
MAC: AA:BB:CC:DD:EE:01  -->  Always assigned to VLAN 10
MAC: AA:BB:CC:DD:EE:02  -->  Always assigned to VLAN 20
```

When a frame arrives, the switch reads the Source MAC address and looks it up in the database to determine VLAN membership.

**Advantage:** The user can plug into any physical port and always land in their correct VLAN.  
**Disadvantage:** More complex to manage. MAC addresses can be spoofed.

### Method 3: Tagged Ingress

If a frame arrives at a Trunk Port already carrying an 802.1Q tag (from another switch), the switch reads the **VID field** directly from the tag to determine VLAN membership. No lookup is needed — the information is already in the frame.

## 11. The Native VLAN

### 11.1 Definition

On a **Trunk Port**, one specific VLAN is designated as the **Native VLAN**. Traffic belonging to the Native VLAN is sent across the trunk **without a VLAN tag**.

All other VLANs on that trunk are sent with their 802.1Q tags intact.

### 11.2 Why the Native VLAN Exists

The Native VLAN exists for backward compatibility. Some older network devices do not understand 802.1Q tagging. If a trunk port sends a tagged frame to such a device, the device cannot process it. By designating one VLAN as "native" (untagged), older devices can still receive and understand that VLAN's traffic.

```
Trunk Port sending frames:

VLAN 10 frame  -->  [tagged:  VID=10 | payload]
VLAN 20 frame  -->  [tagged:  VID=20 | payload]
VLAN 1 frame   -->  [untagged: payload only]   <-- Native VLAN = 1
VLAN 30 frame  -->  [tagged:  VID=30 | payload]
```

### 11.3 Security Note

By default on most switches, VLAN 1 is the Native VLAN. In secure environments, the Native VLAN is changed to an unused VLAN ID that no user devices belong to. This prevents a class of attack called **VLAN hopping**, where an attacker exploits the untagged nature of the Native VLAN to send traffic into other VLANs.

## 12. Minimum Frame Size and Padding

### 12.1 The 64-Byte Minimum Rule

Every Ethernet frame must be **at least 64 bytes long** (not counting the Preamble and SFD fields that exist at the physical layer).

**Why this rule exists:** This minimum size is a requirement of the **CSMA/CD** (Carrier Sense Multiple Access with Collision Detection) protocol. In a shared network medium, two devices can transmit simultaneously, causing a **collision**. For the transmitting device to detect that a collision occurred, it must still be transmitting when the collision signal returns from the furthest point in the network. The 64-byte minimum ensures the frame is large enough for this detection to work correctly.

### 12.2 Padding

If the upper layer (e.g., the IP layer) provides a payload smaller than the minimum required, the system adds **Padding** — zero-value bytes — to bring the frame up to the 64-byte threshold.

Padding is not a fixed field. It only exists when needed and its length varies.

### 12.3 Minimum Frame Calculation: Untagged vs. Tagged

**Untagged Frame:**

```
+-------------+-------------+-----------+------------------+---------+-------+
|  Dest MAC   |   Src MAC   | EtherType |     Payload      | Padding |  FCS  |
|   6 Bytes   |   6 Bytes   |  2 Bytes  |   ???? Bytes     |  Var.   | 4 B   |
+-------------+-------------+-----------+------------------+---------+-------+

Total header overhead: 6 + 6 + 2 = 14 bytes
FCS:                             = 4 bytes
Minimum payload required:   64 - 14 - 4 = 46 bytes

Formula: 14 (header) + 46 (min payload) + 4 (FCS) = 64 bytes MINIMUM
```

**Tagged Frame (802.1Q):**

```
+-------------+-------------+============+-----------+------------------+-------+
|  Dest MAC   |   Src MAC   | 802.1Q TAG | EtherType |     Payload      |  FCS  |
|   6 Bytes   |   6 Bytes   |  4 Bytes   |  2 Bytes  |   ???? Bytes     | 4 B   |
+-------------+-------------+============+-----------+------------------+-------+

Total header overhead: 6 + 6 + 4 + 2 = 18 bytes
FCS:                                   = 4 bytes
Minimum payload required:   64 - 18 - 4 = 42 bytes

Formula: 18 (header) + 42 (min payload) + 4 (FCS) = 64 bytes MINIMUM
```

### 12.4 The Padding Effect of Adding a Tag

| Scenario | Header | Min Payload | FCS | Total |
| :--- | :--- | :--- | :--- | :--- |
| Untagged frame | 14 bytes | **46 bytes** | 4 bytes | 64 bytes |
| Tagged frame | 18 bytes | **42 bytes** | 4 bytes | 64 bytes |

**Conclusion:** Adding a 4-byte VLAN tag does not increase the minimum frame size. The total stays at 64 bytes. However, the 4 bytes of tag now occupy space that payload used to need. This means **4 fewer bytes of padding** are required for a tagged frame to reach the minimum size.

### 12.5 Special Case: Adding a Tag to an Already-Minimum-Size Frame

If a switch receives a 64-byte untagged frame on an Access Port and must forward it out a Trunk Port (adding a 4-byte tag), the result is a **68-byte frame**. This is above the 64-byte minimum, so no adjustment is needed. The frame simply grows by 4 bytes.

### 12.6 Warning for Low-Level Software Development

When writing a network driver or protocol parser that adds VLAN tags:

- The frame length must be **recalculated** after inserting the 4-byte tag.
- If the length is calculated incorrectly, the driver may append random bytes from memory as "padding." This is called **garbage padding** and can cause two problems:
  1. **FCS mismatch:** The checksum is computed over incorrect data, causing the frame to be dropped.
  2. **Memory leak:** Private memory contents (passwords, keys) may be transmitted over the network inadvertently.

## 13. MTU and the "Giant Frame" Problem

### 13.1 What Is MTU?

**MTU** stands for **Maximum Transmission Unit**. It defines the largest payload (data portion) that can be carried in a single Ethernet frame.

| Frame Type | Maximum Total Size |
| :--- | :--- |
| Standard Ethernet (untagged) | 1518 bytes |
| 802.1Q Tagged Ethernet | 1522 bytes |

### 13.2 The Problem

A standard untagged frame has a maximum size of 1518 bytes. When a 4-byte VLAN tag is inserted, the frame becomes 1522 bytes — 4 bytes larger than the standard maximum.

Older networking equipment is programmed to drop any frame larger than 1518 bytes as a "Giant frame" or "Jumbo frame" error — treating it as malformed traffic.

### 13.3 The Solution

Modern networking hardware is built to recognize 802.1Q tags and accept the 1522-byte maximum as valid. This is part of the 802.1Q standard specification.

**For software developers:** When building a network frame parser, the parser must not hardcode `1518` as the maximum allowed frame size. The correct approach is:

1. Check if the TPID is `0x8100`.
2. If yes, accept up to 1522 bytes as valid.
3. Read the EtherType from byte offset 16 (not 12, because 4 bytes have been inserted).

```
Untagged frame byte layout:
Byte 0-5:   Destination MAC
Byte 6-11:  Source MAC
Byte 12-13: EtherType       <-- EtherType is at offset 12

Tagged frame byte layout:
Byte 0-5:   Destination MAC
Byte 6-11:  Source MAC
Byte 12-13: TPID (0x8100)
Byte 14:    PCP + DEI + VID (high bits)
Byte 15:    VID (low bits)
Byte 16-17: EtherType       <-- EtherType is at offset 16 (shifted by 4)
```

## 14. Security Considerations

### 14.1 VLANs Are Not a Firewall

A VLAN provides Layer 2 isolation. This means:

- Devices in different VLANs **cannot directly exchange Ethernet frames**.
- VLANs do **not** provide encryption — the data inside each frame is still unencrypted.
- VLANs do **not** prevent an attacker who gains access to the physical switch from reading all traffic.

VLANs are a segmentation tool, not a substitute for firewalls, encryption, or strong access controls.

### 14.2 Trunk Port Security

A device connected to a Trunk Port can receive frames from every VLAN allowed on that trunk. This makes Trunk Ports a high-value target.

**Rule:** End-user PCs must never be connected to Trunk Ports. Trunk Ports are reserved for:
- Other switches
- Routers and firewalls
- Virtualization hosts that need to connect to multiple VLANs

### 14.3 The 802.1Q Tag NIC Requirement

For a device connected to a Trunk Port to read and process tagged frames, its **Network Interface Card (NIC)** must be configured to handle 802.1Q tagging. If a standard, unconfigured NIC receives a tagged frame, it will see `0x8100` where it expects a known EtherType, fail to recognize it, and discard the frame.

This is actually a passive security feature: unauthorized devices accidentally connected to a trunk port will not automatically gain access to all VLANs — they will simply receive traffic they cannot decode.

## 15. Summary Reference Tables

### 15.1 802.1Q Tag Fields (Complete Reference)

| Field | Size | Value Range | Purpose |
| :--- | :--- | :--- | :--- |
| TPID | 16 bits | Always `0x8100` | Signals this frame has a VLAN tag |
| PCP | 3 bits | 0–7 | Traffic priority (QoS) |
| DEI | 1 bit | 0 or 1 | Drop eligibility during congestion |
| VID | 12 bits | 0–4095 (4094 usable) | VLAN membership ID |

### 15.2 Frame Size Reference

| Frame Type | Header | Min Payload | Max Payload | FCS | Min Total | Max Total |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Untagged | 14 bytes | 46 bytes | 1500 bytes | 4 bytes | 64 bytes | 1518 bytes |
| Tagged (802.1Q) | 18 bytes | 42 bytes | 1500 bytes | 4 bytes | 64 bytes | 1522 bytes |

### 15.3 Port Type Quick Reference

| Feature | Access Port | Trunk Port |
| :--- | :--- | :--- |
| Number of VLANs | 1 | Many |
| Connected to | End devices | Switches, Routers |
| Sends tagged frames | No | Yes |
| Receives tagged frames | No | Yes |
| Device VLAN awareness | Not required | Required |
| Native VLAN applicable | No | Yes |

### 15.4 VLAN Assignment Methods

| Method | Basis of Assignment | Best Use Case |
| :--- | :--- | :--- |
| Static (Port-Based) | Physical port number | Fixed, predictable environments |
| Dynamic (MAC-Based) | Device MAC address | Mobile users, hot-desking |
| Tagged Ingress | 802.1Q VID in frame | Switch-to-switch connections |

### 15.5 Three Core Benefits of VLANs

| Benefit | Mechanism |
| :--- | :--- |
| **Performance** | Broadcasts are limited to the ports within a single VLAN |
| **Security** | Layer 2 isolation prevents direct communication between VLANs |
| **Cost** | Multiple logical networks share one physical switch |

> **Revision Note:** This document covers IEEE 802.1Q VLAN tagging, port types (Access and Trunk), inter-VLAN routing requirements, frame structure, padding rules, MTU implications, and low-level parsing considerations. All concepts are self-contained and require no external references.
