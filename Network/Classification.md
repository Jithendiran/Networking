## Evolution of IP Address Management: The Network/Host Split

The total IPv4 address space ranges from `0.0.0.0` to `255.255.255.255`. Using this range as a single flat list is inefficient for global communication. To manage data delivery, addresses must be divided into smaller, manageable groups.

### 1. The Core Requirement: Network and Host Identification
Every IP address must serve two distinct purposes simultaneously:
* Network Identifier: Identifies the specific network (the group) to which a device belongs.
* Host Identifier: Identifies the specific device (the individual) within that network.

The "Why": Routers cannot store the location of every individual device on Earth. Instead, routers store the location of networks. When a packet arrives, the router looks at the Network Identifier to send it to the correct destination network. Once the packet reaches that network, the Host Identifier is used to find the specific device.

### 2. The Problem of Scale
Organizations vary in size, creating different requirements for address counts:
* Large Organizations: Require a small portion of the address for the Network ID and a large portion for Host IDs (supporting millions of devices).
* Small Organizations: Require a large portion for the Network ID and a small portion for Host IDs (supporting only a few dozen devices).

If every organization received the same fixed split, small organizations would waste millions of addresses, and large organizations would not have enough.

### 3. The Efficiency Challenge: The "Split Point"
A mechanism is required to define exactly where the Network Identifier ends and the Host Identifier begins within the 32-bit IP address.
* Variable Split Complexity: If the split point is unique for every single organization, routers would need to store the IP address PLUS an extra piece of metadata for every entry in a routing table. This increases memory usage and slows down the lookup process.
* The 1981 Solution (Self-Encoding): To maintain speed and simplicity, engineers decided the split point must be encoded within the address itself. By examining the first few bits of an IP address, a router can immediately determine the boundary between the Network ID and the Host ID.

### 4. Transition to Fixed-Class Logic

The decision to encode the split within the bits led to the creation of a system where the address value dictates the network size. This removed the need for routers to carry external information regarding the split point for every individual address.

This became the classful addressing system.

## Classful Addressing
Classful addressing was the first implementation of the "self-encoding" split point. Under this system, the first few bits of an IP address determine exactly where the network portion ends and the host portion begins. This allowed routers to identify the network size without requiring any additional information.


### The Structure of Classes
The address space is divided into five classes (`A`, `B`, `C`, `D`, and `E`). The first three classes (`A`, `B`, and `C`) are used for standard host-to-host communication.

| Class | Leading Bits | Network / Host Split | Number of Networks | Hosts per Network |
| :--- | :--- | :--- | :--- | :--- |
| **Class A** | `0` | 8 bits / 24 bits | 128 | 16,777,214 |
| **Class B** | `10` | 16 bits / 16 bits | 16,384 | 65,534 |
| **Class C** | `110` | 24 bits / 8 bits | 2,097,152 | 254 |

### Class A: Large-Scale Networking

Class A was established to provide a massive number of host addresses to a small number of entities. By allocating 24 bits to the host portion, it allows global organizations or internet service providers to manage millions of devices under a single network prefix without needing multiple network IDs.

`0nnnnnnn.hhhhhhhh.hhhhhhhh.hhhhhhhh`: `n` $\rightarrow$ network and `h` $\rightarrow$ host 


#### 1. Structure and Identification
An IPv4 address consists of 32 bits divided into four 8-bit octets. In Class A, the split occurs after the first octet.

* **The Binary Rule:** The first bit of the first octet is fixed as **0**. 
* **The Network ID:** The first 8 bits (first octet).
* **The Host ID:** The remaining 24 bits (last three octets).

#### 2. Network Range Logic
Because the first bit is fixed at `0`, the first octet can only range from `00000000` (0) to `01111111` (127).

* **Total Mathematical Networks:** $2^7 = 128$
* **Reserved Networks:** Two networks are subtracted from the total because they serve special functions:
    * **0.x.x.x (Network 0):** Reserved for "this network" or default routing. It cannot be assigned to a physical network.
    * **127.x.x.x (Network 127):** Reserved for **Loopback**. Data sent to any address starting with 127 stays within the local device for software testing.
* **Usable Networks:** $128 - 2 = 126$. The usable range is **1.x.x.x to 126.x.x.x**.
    * Check [Reserve](#refined-calculation-of-usable-public-networks) for calculation more detail

#### 3. Host Capacity Logic
Each Class A network uses $32-8\text{(network)}=24$, 24 bits for host addresses. 

* **Total Mathematical Addresses:** $2^{24} = 16,777,216$
* **Usable Hosts per Network:** $16,777,216 - 2 = 16,777,214$.
* **The "Minus 2" Requirement:** Every network requires two addresses for management, which cannot be assigned to devices:
    1.  **Network Address:** All host bits are set to **0**. (Example: `10.0.0.0`). This identifies the network itself.
    2.  **Broadcast Address:** All host bits are set to **1**. (Example: `10.255.255.255`). This is used to send data to every device on that specific network simultaneously.


#### 4. Broadcast Address
A common error is assuming `255.255.255.255` is the only broadcast address. Every individual network has its own unique broadcast address.

* If the network is `1.0.0.0`, the broadcast is `1.255.255.255`.
* If the network is `10.0.0.0`, the broadcast is `10.255.255.255`.
* **Note:** In Class A, the broadcast only occurs when **all 24 host bits** (the last three octets) are 255.

#### 5. The Mechanical Logic of Bit-Matching

In early computing, **routers** did not have sophisticated software to look at every digit of an IP address. They operated on hard-wired logic gates. These gates were designed to look at the very beginning of a 32-bit string and make an instantaneous decision.

##### 1. The "Off-Switch" (Network 0)
When a router sees a `0` as the first octet, it is essentially a signal that says:
* Rule: "This message is not going to a specific person on the internet; it is a general request from a device that doesn't have an identity yet."
* The Efficiency Gain: By reserving the entire block (everything starting with 0), the router only has to look at the very first octet. If that octet is 0, the router immediately knows the packet is "local" or "unassigned" and stops processing it for the global internet.
* The "Waste": Because the router's brain stops checking after that first 0, it cannot distinguish between 0.0.0.0 and 0.5.5.5. To the router, they both just look like a "blank" start.

##### 2. The "Internal Mirror" (Network 127)
The Loopback network functions as a "virtual" wire inside the computer.
* The Reason for the Whole Block: Software developers often need more than one "local" address to test how different programs on the same computer talk to each other.
* Hardware Simplification: If the system only reserved 127.0.0.1, the computer's Network Interface Card (NIC) would have to compare all 32 bits of every outgoing packet to see if it should be sent out or kept inside. By reserving the entire 127 block, the NIC only looks at the first 8 bits or 1st octet. If they match 127, the NIC "loops" the data back to the CPU immediately.

#### 6. How do Class B/C devices perform testing or initialization?
Every device on a network—regardless of whether it has been assigned a Class A, B, or C address—has a TCP/IP Stack (the software that handles networking). This software is programmed to recognize the reserved Class A ranges for specific tasks:

* For Testing (Loopback): Even if a computer is assigned the Class C address 192.168.1.50, it still uses the Class A address 127.0.0.1 to talk to itself. The computer simply "reaches over" into the reserved Class A loopback range for that specific internal function.
* For Initialization (Network 0): When a new device (like a laptop) first joins a network and doesn't have an IP yet, it identifies itself as 0.0.0.0 to ask the network for a real address. It uses this "Class A" reserved value as a temporary placeholder until it receives its permanent Class B or C address.

Summary: Reserved addresses are like public emergency tools. Even if you live in a Class C "house," you still use the same Class A "emergency phone" (0.0.0.0) and "private diary" (127.0.0.1) as everyone else.

#### 7. Class A Scarcity and Ownership

##### 1. The "126 Companies" Reality
In the original Classful system, there were only 126 usable Class A networks available globally (1 to 126).
* The Allocation Policy: Because each Class A network contains over 16 million addresses, they were only given to the largest entities in the world during the early days of the internet (1980s and early 1990s).
* The "Owners": These 126 slots were filled by:
    - Major Corporations: (e.g., IBM, Apple, Ford, General Electric).
    - Governments: (e.g., The U.S. Department of Defense).
    - Research Institutions: (e.g., MIT, Stanford).
    - Early ISPs: Entities that distributed the addresses to smaller customers.

##### 2. The Problem of "The 127th Company"
If a 127th giant corporation appeared and needed 10 million addresses, there were no Class A slots left. This led to the Efficiency Crisis:
* Forcing Class B: That company would have to be given many Class B networks instead.
* Fragmentation: Managing 200 Class B networks is much harder for a router than managing one Class A network.
* Exhaustion: This rigid limit (only 126 "big" slots) is exactly why the Classful system was eventually replaced. It created a world where a few organizations "owned" almost half of the entire internet, while everyone else had to fight for the remaining Class B and C space.


### Class B: Medium-to-Large Scale Networking
Class B was designed for organizations that require more than the 254 addresses provided by Class C, but do not require the millions of addresses provided by Class A. It balances the number of available networks with the number of hosts per network.

`10nnnnnn.nnnnnnnn.hhhhhhhh.hhhhhhhh`: `n` $\rightarrow$ network and `h` $\rightarrow$ host 

#### 1. Structure and Identification
In Class B, the 32-bit address is split exactly in the middle.
* The Binary Rule: The first two bits are fixed as `10`. This bit pattern acts as a "hard-wired" signal to routers that the first 16 bits represent the network.
* The Network ID: The first 16 bits (the first two octets).
* The Host ID: The remaining 16 bits (the last two octets).

#### 2. Network Range Logic
Because the first two bits are fixed as `1` and `0`, the value of the first octet is restricted.
* Binary Range: `10000000` (128) to `10111111` (191).
* Decimal Range: 128.x.x.x to 191.x.x.x.
* Total Mathematical Networks: $2^{14} = 16,384$. (Calculated as 14 bits because 2 of the 16 network bits are fixed).
* Check [Reserve](#refined-calculation-of-usable-public-networks) for calculation more detail

#### 3. Host Capacity Logic
Class B uses 16 bits for the host portion.
* Total Mathematical Addresses: $2^{16} = 65,536$
* Usable Hosts per Network: $65,536 - 2\text{ Network + Broadcast for the range} = 65,534$.
* The "Why" of Capacity: This size was ideal for universities and large regional hospitals, providing enough addresses for every desktop and server on a single campus.

### Class C: Small-Scale Networking
Class C was designed for small businesses and local area networks (LANs). It prioritizes having a very large number of available networks, with each network containing a small number of hosts.

`110nnnnn.nnnnnnnn.nnnnnnnn.hhhhhhhh`: `n` $\rightarrow$ network and `h` $\rightarrow$ host 

#### 1. Structure and Identification
In Class C, the split occurs after the third octet.
* The Binary Rule: The first three bits are fixed as `110`. This identifies the address as a small network immediately upon hardware inspection.
* The Network ID: The first 24 bits (the first three octets).
* The Host ID: The remaining 8 bits (the last octet).

#### 2. Network Range Logic
With the first three bits fixed as `110`, the first octet range is determined as follows:
* Binary Range: `11000000` (192) to `11011111` (223).
* Decimal Range: 192.x.x.x to 223.x.x.x.
* Total Mathematical Networks: $2^{21} = 2,097,152$. (21 bits are variable within the 24-bit network portion).
* Check [Reserve](#refined-calculation-of-usable-public-networks) for calculation more detail

#### 3. Host Capacity Logic
Class C uses only 8 bits for the host portion.
* Total Mathematical Addresses: $2^8 = 256$
* Usable Hosts per Network: $256 - 2 = 254$.
* The "Why" of Efficiency: Most small offices in the 1980s and 90s did not exceed 200 devices. Class C allowed the internet to be divided into millions of these small "neighborhoods" without wasting the massive address blocks seen in Class A.

### BroadCast
#### 1. Network Broadcast (Directed Broadcast)
A Network Broadcast is an address used to send data to every host within a specific network. While the network portion of the address remains fixed, the host portion is set entirely to binary 1s.
* The Logic: This address allows a sender located on a different network to target a specific group of devices. For example, a router in New York can send a packet to every device in a branch office in London by using that specific network's broadcast address.
* Structure:
    - Network Portion: Matches the target network.
    - Host Portion: All bits are set to 1 (Decimal 255).
    - Example (Class C): In the network 192.168.1.0, the network broadcast is 192.168.1.255.
#### 2. All-Ones Broadcast (Limited Broadcast)
The All-Ones Broadcast is the address 255.255.255.255. It is used to communicate with every device on the local network segment only.
* The Logic: 
    - This is used when a device does not yet know its own IP address or the identity of the network it has joined (e.g., a computer asking a DHCP server for an IP address). Because the device has no network information, it cannot "aim" at a specific network broadcast address. It simply "shouts" to everything physically connected to it. 
    - Routers are programmed to never pass a 255.255.255.255 packet to another network. This prevents a single device from accidentally "shouting" to the entire global internet.
* Structure: `255.255.255.255`

| Feature | Network Broadcast (Directed) | All-Ones Broadcast (Limited) |
| :--- | :--- | :--- |
| **Address Value** | Specific to the network (e.g., `10.255.255.255`). | Always `255.255.255.255`. |
| **Scope** | Targets a **specific** remote or local network. | Targets **only** the immediate local segment. |
| **Router Behavior** | Routers **can** be configured to forward these across networks. | Routers **never** forward these; they are dropped at the boundary. |
| **Primary Use** | Remote management or sending data to a known subnet. | Initial configuration (DHCP) or local discovery (ARP). |
| **Requirement** | The sender must know the network's structure. | The sender requires zero knowledge of the network. |

## Special-Purpose Classes: Class D and Class E
While Classes A, B, and C are designed for standard communication where one sender talks to one receiver (Unicast), the remaining portion of the IPv4 address space is reserved for specialized functions. These classes do not follow the Network/Host split logic because they do not identify individual devices on a network.

| Class | Leading Bits | Decimal Range | Purpose |
| :--- | :--- | :--- | :--- |
| **Class D** | `1110` | 224.0.0.0 to 239.255.255.255 | Multicast Groups |
| **Class E** | `1111` | 240.0.0.0 to 255.255.255.255 | Experimental / Research |

### Class D: Multicast Addressing
Class D is dedicated to Multicast. In standard (Unicast) networking, a packet has one destination. In Multicast, a single packet is sent to a group of interested receivers simultaneously.

#### 1. The Logic of Multicast
Multicast exists to reduce bandwidth waste. If 1,000 users want to watch the same live video stream, a Unicast server would have to send 1,000 separate copies of the data. With Class D, the server sends one copy to a Class D address, and the network hardware duplicates the data only where necessary to reach the subscribers.

#### 2. Absence of Host IDs
Class D addresses do not represent "hosts." Instead, they represent groups or services.
* The "Channel" Concept: A Class D address acts like a television channel. A device "tunes in" to a Class D address to receive specific data. Because no single device "owns" the address, there is no host portion and no "broadcast" address within Class D.

#### 3. Binary Identification
* The Binary Rule: The first four bits are fixed as `1110`.
* Range Calculation: The lowest value is `11100000` (224) and the highest is `11101111` (239).

[ClassD](./classd.md)

### Class E: Experimental and Reserved
Class E was set aside by the Internet Engineering Task Force (IETF) for future use, research, and development.

#### 1. The Purpose of Reservation
When IPv4 was designed, engineers reserved this block to ensure that if the internet required a new type of routing or management protocol in the future, there would be a "blank space" in the address range to test and implement those changes without interfering with existing traffic.
#### 2. Operational Status
In modern networking, Class E addresses are generally unusable for standard internet traffic. Most operating systems and routers are programmed to drop packets using these addresses as "invalid."
* Exception: The address 255.255.255.255 technically falls within the Class E range but is reserved for the Limited Broadcast function described in previous sections.

#### 3. Binary Identification
* The Binary Rule: The first four bits are fixed as 1111.
* Range Calculation: The lowest value is 11110000 (240) and the highest is 11111111 (255).


## Definition and Purpose of Private Addresses

A **Private Address** is a specific IP address reserved for use within a local area network (LAN) that is not reachable from the global internet.

### 1. The "Why": Address Conservation and Security
The IPv4 pool contains a finite number of addresses (approximately 4.3 billion). If every lightbulb, smartphone, and laptop on Earth required a unique public address, the supply would have been exhausted decades ago. 

* **Conservation:** Private addresses allow millions of devices to share a single public IP address through a process called Network Address Translation (NAT). For example, 500 computers in an office can all use private addresses internally, while the office router uses only one public address to communicate with the internet.
* **Security:** Because private addresses are "non-routable," external devices on the internet cannot send data directly to a private IP. This creates a natural barrier that protects internal devices from direct external access.

### 2. The Logic of Non-Routability
Internet Service Providers (ISPs) and global routers are programmed to ignore any traffic that has a private address as its destination. If a packet with a destination of `10.0.0.1` reaches the public internet, it is immediately destroyed (dropped). This ensures that internal traffic stays internal.


## Automatic Private IP Addressing (APIPA)
Automatic Private IP Addressing (APIPA) is a localized addressing feature that enables a device to self-assign an IP address when it cannot obtain one from a central server. It functions as a "fail-safe" mechanism to ensure local communication remains possible even in the absence of a network administrator or automated configuration service.

### 1. The "Why": The Problem of Connection Failure
In modern networks, devices typically receive their IP addresses from a Dynamic Host Configuration Protocol (DHCP) server. This server acts as a centralized manager that hands out unique addresses to every device that joins.
* The Failure Scenario: If the DHCP server crashes, the network cable is damaged, or the server runs out of addresses, a joining device remains "nameless." Without an IP address, the device's TCP/IP stack cannot function, and it cannot communicate with any other device on the wire.
* The APIPA Solution: Instead of remaining disconnected, the device recognizes the lack of a DHCP response and assigns itself a temporary address from a globally reserved range. This allows the device to talk to other devices on the same physical segment that are also in "fail-safe" mode.

### 2. The Reserved Range: Class B Block
APIPA uses a specific range of addresses reserved by the Internet Assigned Numbers Authority (IANA) for local use only.
* The Range: 169.254.0.1 to 169.254.255.254.
* Classification: APIPA resides within the Class B space.
* The Mask: It uses a default Class B mask of 255.255.0.0. This means the first two octets (169.254) identify the "emergency" network, while the last two octets are used for individual host identification.

### 3. Operational Logic: How a Device Self-Assigns
Since there is no central server to manage an APIPA network, devices must ensure they do not choose the same address as a neighbor.
1. Detection: The device sends out a request for a DHCP server. If no response is received after a set period (usually 1–2 minutes), APIPA is triggered.
2. Selection: The device randomly selects an address within the 169.254.x.x range.
3. Conflict Check (ARP): The device sends an Address Resolution Protocol (ARP) packet to the local network asking, "Does anyone own this address?"
    * If a Conflict Exists: If another device responds, the first device picks a different random number and repeats the check.
    * If No Conflict: The device assumes the address and begins operation.
4. Background Monitoring: While using the APIPA address, the device continues to check for a DHCP server in the background. If a server becomes available later, the device will discard the APIPA address and adopt the official address provided by the server.

### 4. Critical Constraints and Limitations
The APIPA system is designed for emergency or small-office/home-office (SOHO) use. It has two major functional limitations:
* Non-Routable Nature: APIPA addresses are strictly for local communication. Routers are programmed to never forward packets with a source or destination of 169.254.x.x. Consequently, a device with an APIPA address can talk to a printer in the same room but cannot access the Internet or a different office branch.
* Default Gateway Absence: Because there is no central configuration, APIPA does not provide a Default Gateway (the address of the router). Without a gateway, the device has no path to leave its immediate local network.

### 5. Global use
While the APIPA range is mathematically a Class B network, it is a universal service available to all devices, regardless of the class (A or B or C) they are intended to occupy.

#### 1. The Distinction: Address Ownership vs. Service Access
To understand why any device can use APIPA, one must distinguish between a device's Permanent Identity and its Emergency Identity.
* Permanent Identity (A, B, or C): This is the address an administrator intends for the device to have (e.g., a Class C address like 192.168.1.50).
* Emergency Identity (APIPA): This is a built-in software routine. If the Permanent Identity cannot be obtained, the software ignores the intended class and "borrows" the APIPA Class B range to maintain basic functionality.

Just as a person living in a small apartment (Class C) and a person living in a large mansion (Class A) both use the same universal emergency phone number, all devices use the same Class B APIPA range when their primary connection fails.


## IPv4 Comprehensive Address Classification Table

This table summarizes the entire IPv4 space, identifying both the standard ranges and every specific reserved block.

| Class | Range | Status | Purpose / Underlying Logic |
| :--- | :--- | :--- | :--- |
| **Class A** | `0.0.0.0/8` | **Reserved** | **Software Initialization:** Placeholder for a device that does not yet have an IP. |
| **Class A** | `1.0.0.0` – `9.255.255.255` | **Public** | Publicly routable addresses for large organizations. |
| **Class A** | `10.0.0.0/8` | **Private** | **Internal Networking:** Private use within a single organization; not routable on the Internet. |
| **Class A** | `11.0.0.0` – `100.63.255.255` | **Public** | Publicly routable addresses for large organizations. |
| **Class A** | `100.64.0.0/10` | **Reserved** | **Carrier-Grade NAT:** Shared address space for ISPs to manage subscriber networks. |
| **Class A** | `100.128.0.0` – `126.255.255.255`| **Public** | Publicly routable addresses for large organizations. |
| **Class A** | `127.0.0.0/8` | **Reserved** | **Loopback:** Internal testing; traffic never leaves the local network interface. |
|--|--|--|--|
| **Class B** | `128.0.0.0` – `169.253.255.255`| **Public** | Publicly routable addresses for medium-to-large organizations. |
| **Class B** | `169.254.0.0/16` | **Reserved** | **APIPA:** Self-assigned local addressing when DHCP fails. |
| **Class B** | `169.255.0.0` – `172.15.255.255`| **Public** | Publicly routable addresses for medium-to-large organizations. |
| **Class B** | `172.16.0.0/12` | **Private** | **Internal Networking:** Private use for medium organizations. |
| **Class B** | `172.32.0.0` – `191.255.255.255`| **Public** | Publicly routable addresses for medium-to-large organizations. |
|--|--|--|--|
| **Class C** | `192.0.0.0/24` | **Reserved** | **IETF Protocol Assignments:** Reserved for specific network protocol header use. |
| **Class C** | `192.0.2.0/24` | **Reserved** | **Documentation (TEST-NET-1):** For use in textbooks and examples. |
| **Class C** | `192.168.0.0/16` | **Private** | **Internal Networking:** Private use for homes and small offices. |
| **Class C** | `198.18.0.0/15` | **Reserved** | **Benchmark Testing:** Used for measuring performance between networks. |
| **Class C** | `198.51.100.0/24` | **Reserved** | **Documentation (TEST-NET-2):** For use in textbooks and examples. |
| **Class C** | `203.0.113.0/24` | **Reserved** | **Documentation (TEST-NET-3):** For use in textbooks and examples. |
| **Class C** | All other ranges | **Public** | Publicly routable addresses for small organizations. |
|--|--|--|--|
| **Class D** | `224.0.0.0` – `239.255.255.255`| **Reserved** | **Multicast:** One-to-many communication (Streaming, Routing updates). |
|--|--|--|--|
| **Class E** | `240.0.0.0` – `255.255.255.254`| **Reserved** | **Experimental:** Reserved for research and future development. |
| **N/A** | `255.255.255.255` | **Reserved** | **Limited Broadcast:** Targets every host on the immediate local segment. |


### Refined Calculation of Usable Public Networks

To ensure the documentation is technically accurate, the deductions must account for every reserved block within the specific bit-boundaries of each Class.


### 1. Usable Public Class A Networks
The calculation for Class A is the most straightforward because the reservations (0, 10, 100, and 127) align cleanly with the 8-bit network boundaries.

#### Deductions:
1.  **Network 0 (`0.0.0.0/8`):** Software Initialization.
2.  **Network 10 (`10.0.0.0/8`):** Private Networking.
3.  **Network 100 (`100.64.0.0/10`):** Shared Address Space (CGNAT). Since a large portion of this block is reserved, the entire "Network 100" is removed from the standard public pool.
4.  **Network 127 (`127.0.0.0/8`):** Loopback.

#### Final Count:
$$128\text{ (Total)} - 4\text{ (Special Blocks)} = 124$$
**Public Range:** 1.0.0.0 – 9.255.255.255, 11.0.0.0 – 99.255.255.255, 101.0.0.0 – 126.255.255.255.


### 2. Usable Public Class B Networks
In Class B, the first two octets define the network. We must identify exactly which 16-bit blocks are reserved.

#### The "17" Deductions Explained:
To reach the count of 17 reserved networks, the following specific Class B blocks are subtracted from the $16,384$ total:

* **APIPA (1 Network):** 1. `169.254.0.0`
* **RFC 1918 Private Range (16 Networks):** The range is `172.16.0.0` to `172.31.255.255`. In Class B logic, this is a sequence of 16 individual networks:
    1. `172.16.0.0`
    2. `172.17.0.0`
    3. `172.18.0.0`
    4. `172.19.0.0`
    5. `172.20.0.0`
    6. `172.21.0.0`
    7. `172.22.0.0`
    8. `172.23.0.0`
    9. `172.24.0.0`
    10. `172.25.0.0`
    11. `172.26.0.0`
    12. `172.27.0.0`
    13. `172.28.0.0`
    14. `172.29.0.0`
    15. `172.30.0.0`
    16. `172.31.0.0`

#### Final Count:
$$16,384\text{ (Total)} - 17\text{ (Special Networks)} = 16,367$$


### 3. Usable Public Class C Networks
Class C uses 24 bits for the network ID. Deductions here are higher because the private range alone covers a significant number of 24-bit networks.

#### Deductions:
1.  **Private Range (`192.168.0.0/16`):** This block contains **256** individual Class C networks (from `192.168.0.0` to `192.168.255.0`).
2.  **Documentation (TEST-NET-1):** `192.0.2.0` (1 Network).
3.  **IETF Protocol Assignments:** `192.0.0.0` (1 Network).
4.  **Benchmark Testing:** `198.18.0.0/15`. This block covers **2** Class B-sized spaces, but within the Class C range, it consumes **512** Class C networks (from `198.18.0.0` to `198.19.255.0`).
5.  **Documentation (TEST-NET-2/3):** `198.51.100.0` and `203.0.113.0` (2 Networks).

#### Final Count:
$$2,097,152\text{ (Total)} - (256 + 1 + 1 + 512 + 2) = 2,096,380$$

**Note:** The exact number can vary slightly depending on how many specialized protocol blocks (like `192.0.0.0/24`) are strictly excluded from "public" sale by IANA.



### Summary Table for Final Documentation

| Class | Total Networks | Reserved Networks | Publicly Routable Networks |
| :--- | :--- | :--- | :--- |
| **Class A** | 128 | 4 | 124 |
| **Class B** | 16,384 | 17 | 16,367 |
| **Class C** | 2,097,152 | ~772 | ~2,096,380 |

**Conclusion:** The transition from the mathematical total to the usable total is necessary because "Publicly Routable" implies the address must be unique and reachable globally. Reserved blocks are excluded to prevent routing conflicts and ensure stability for specialized functions like internal networking and testing.

**Note on Efficiency:** The primary failure of this system was the rigid "split points." An organization needing 300 addresses was too large for Class C (254 hosts) and was forced to take a Class B (65,534 hosts), resulting in 65,234 wasted addresses. This inefficiency eventually necessitated the move to **Classless Inter-Domain Routing (CIDR)**.