## Internet Protocol Addressing 

### Before IPv4 — Early Attempts
The internet did not begin with IPv4. Before it, two earlier systems were used.

#### NCP (Network Control Protocol) — 1969
The first protocol used on `ARPANET` (the original internet, connecting universities and military). NCP used 8-bit host addresses. This meant only 256 devices could exist on the entire network. There was no concept of networks — just one flat list of devices. As `ARPANET` grew beyond a handful of machines, 256 addresses became insufficient immediately.

```
[ 8 bits ]
  host number only
  e.g. 00001010 = host 1
```

#### IPv1, IPv2, IPv3 — 1973 to 1977
These were experimental designs created during research by Vint Cerf and Bob Kahn. None were deployed publicly. They were paper designs and small experiments, testing different ideas about how addresses should be structured. The version numbers 1, 2, and 3 were used internally in research documents.

During the period of 1973 to 1977, the "Internet" was a research project called ARPANET. Engineers were trying to figure out how to handle two distinct tasks:
1. Transmission Control (TCP): Making sure data arrives without errors.
2. Internetworking (IP): Moving data from one network to another using addresses.

In versions 1, 2, and 3, these two tasks were combined into a single protocol. There was no "IP address" separate from a "TCP header." These versions used a small Network ID and a Host ID, but the total length was inconsistent, which crashed early networking hardware.

##### IPv1 and IPv2 (1973–1976)
* How they looked: They used variable-length addresses. The address format was not strictly defined as 32-bit; instead, it was part of a larger "TCP Header."
* The Structure: The address was often just a simple "Host Number" within a specific network.
* Why they were dropped: 
    * Complexity: Because the address length could change, routers struggled to process the headers quickly.
    * Lack of Separation: Since the addressing and the error-checking were one single block of code, you couldn't change how a device was addressed without breaking how the data was checked for errors.

##### IPv3 (1977)
* How it looked: Version 3 began to look closer to what we use today. It attempted to separate the "Network ID" from the "Host ID," but it was still technically part of the TCPv3 protocol.
* The Structure: It experimented with different bit lengths for the address field to see how many networks the world might eventually need.
* Why it was dropped:
    * The "Great Split": In 1978, the lead designers (Vint Cerf and Jon Postel) realized that moving data (IP) and ensuring reliability (TCP) should be two different layers.
    * The Birth of IPv4: They decided to split the protocol. They finalized the address size at 32 bits to provide a balance between enough addresses and processing speed for the hardware of that time. This split resulted in what we now call TCP/IP, and the first version of the standalone IP protocol was numbered Version 4.

```
+-------------------------------------------------------------+
|                         HEADER                              |
+---------+---------+----------------+------------------------+
| Version |  Flags  | Source Address | Destination Address    |
| (1 or 2)|         | (Variable Size)| (Variable Size)        |
+---------+---------+----------------+------------------------+
|        Sequence Numbers / Error Checking / Data Content     |
+-------------------------------------------------------------+
```

##### IPv4 (1983)
IPv4 was the first stable, split-layer protocol, every hardware manufacturer built their devices to follow the IPv4 rules.

**Key Factors of IPv4 Success:**
* Fixed Length: Every address is exactly 32 bits. This allows hardware (Routers) to process packets at extremely high speeds because they always know exactly where the address starts and ends in the data stream.
* Separation of Concerns: By making IP its own layer, the internet can carry any type of data (web pages, emails, video) without the router needing to understand what the data is. The router only looks at the 32-bit IP address.

###### Address Representation
A 32-bit binary number (like `11000000101010000000000100000001`) is difficult for humans to read. IPv4 introduced Dotted Decimal Notation.
* The 32 bits are divided into four 8-bit sections called octets.
* Each octet is converted into a decimal number between 0 and 255.
* Example: 192.168.1.1

###### Core Concepts: Network and Host
IPv4 clearly separates an address into two parts
* The Network ID: Identifies the specific "street" or group the device belongs to.
* The Host ID: Identifies the specific "house" or device on that street.

The Subnet Mask: This is a companion number used to tell the computer where the Network ID ends and the Host ID begins.

###### Communication Types
IPv4 defined specific ways to send data, which did not exist in a standardized way in IPv1-v3:
* Unicast: One-to-One. Sending data from one specific computer to another.
* Broadcast: One-to-All. Sending data to every device on a specific network. The address `255.255.255.255` is used for this.
* Multicast: One-to-Many. Sending data to a specific group of interested devices (e.g., a video stream). Addresses starting with `224.x.x.x` through `239.x.x.x` are reserved for this.

###### Special Address
* Loopback (127.0.0.1): Used by a computer to talk to itself for testing.
* Private Addresses: Specific ranges (like 192.168.x.x) used for internal home or office networks. These addresses cannot be seen directly on the public internet.

### IPv4

#### Header
The IPv4 header is a piece of information attached to the front of every data packet. It acts as the "envelope" that contains the delivery instructions. In IPv4, this header is usually 20 bytes (160 bits) long.

The header is organized into rows of 32 bits each. This allows hardware to read the information in consistent chunks.

* Row 1: Basic Control
    - Version (4 bits): Always set to 4. This tells the receiver to use IPv4 rules.
    - IHL (Internet Header Length) (4 bits): Tells the receiver how long the header is.
    - Type of Service (8 bits): Used to prioritize certain data (e.g., making a voice call higher priority than a file download).
    - Total Length (16 bits): The size of the entire packet (header + actual data).
* Row 2: Fragmentation (Breaking data into smaller pieces)
    - Identification (16 bits): A unique ID number for the packet.
    - Flags (3 bits): Instructions on whether the packet is allowed to be broken into smaller pieces (fragmented).
    - Fragment Offset (13 bits): If the packet was broken up, this number tells the receiver where this specific piece fits in the original sequence.
* Row 3: Survival and Protocol
    - Time to Live (TTL) (8 bits): A "timer" to prevent packets from wandering the internet forever. Every time a packet hits a router, this number drops by 1. If it hits 0, the packet is deleted.
        - The Time to Live (TTL) is not a "timeout" in terms of minutes or seconds. Instead, it is a hop counter. It limits the lifespan of a packet based on how many routers it passes through.
        - In networking, errors can occur where a packet gets stuck in a Routing Loop. Imagine Router A thinks the best path to a destination is through Router B. Meanwhile, Router B thinks the best path is back through Router A. Without TTL, the packet would bounce between these two routers forever, consuming bandwidth and eventually crashing the network.
        - The Hop: Every time the packet arrives at a router, that router performs a simple math operation: $TTL = TTL - 1$.
    - Protocol (8 bits): Tells the receiver which protocol is inside the packet (e.g., TCP or UDP).
    - Header Checksum (16 bits): A math calculation used to check if the header was corrupted during travel.
* Row 4: The Origin
    - Source IP Address (32 bits): The IP address of the device sending the data.
* Row 5: The Destination
    - Destination IP Address (32 bits): The IP address of the device intended to receive the data.

This diagram represents how the 160 bits (20 bytes) are arranged.
```
0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  |Type of Service|          Total Length         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification        |Flags|      Fragment Offset    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Time to Live |    Protocol   |         Header Checksum       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source IP Address                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination IP Address                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options (if any)                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Address
An Internet Protocol version 4 (IPv4) address is a **32-bit binary number**. This number functions as a unique identifier for a device on a network. To make this 32-bit string manageable for hardware and readable for humans, it is organized into a specific hierarchy.

##### 1. The Binary Foundation (The Machine View)
At the lowest level, an IP address is a continuous sequence of 32 bits. A **bit** is the smallest unit of data, representing either a `0` or a `1`.

* **Structure:** `bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb`
* **Total Combinations:** Because there are 32 positions, there are $2^{32}$ possible unique addresses, totaling **4,294,967,296**.

##### 2. The Octet Division (The Logic View)
Processing 32 bits as a single block is computationally inefficient for early networking hardware. To simplify management, the 32 bits are divided into four equal segments called **octets**.

* **Definition of an Octet:** A grouping of **8 bits**.
* **Structure:** `bbbbbbbb.bbbbbbbb.bbbbbbbb.bbbbbbbb`
* **Logic:** By breaking the address into octets, networking equipment can process the address one byte (8 bits) at a time.


##### 3. Dotted Decimal Notation (The Human View)
Binary strings are difficult for humans to read and remember. Therefore, each 8-bit octet is converted into a decimal number.

* **The Range of an Octet:** * The lowest value is `00000000`, which equals **0**.
    * The highest value is `11111111`, which equals **255** ($128+64+32+16+8+4+2+1$).
* **The Format:** The four decimal numbers are separated by periods (dots).
* **Representation:** `X.X.X.X` (where each `X` is a value from 0 to 255).

##### 4. Summary of Components
The following table illustrates how a single address is represented across different formats:

| Format | Example Representation |
| :--- | :--- |
| **Binary** | `11000000.10101000.00000001.00000001` |
| **Bit Count** | 8 bits . 8 bits . 8 bits . 8 bits (Total: 32) |
| **Dotted Decimal** | `192.168.1.1` |
| **Octet Range** | Each segment ranges from `0` to `255` |

##### The "Why": Precision and Constraint
The reason an octet cannot exceed **255** is a mathematical constraint of the 8-bit limit. Allowing a number higher than 255 would require a 9th bit, which would break the 32-bit standardized structure of the IPv4 protocol. This rigid mathematical framework ensures that every router in the world interprets the "start" and "end" of an address in the exact same way.

**Summary**: 
An IPv4 address is exactly 32 bits long. These bits are divided into 4 equal groups called Octets.
* 1 Octet = 8 bits.
* Total bits = $8 \times 4 = 32$.

Each octet can represent a number from 0 to 255.
* Minimum: 00000000 (Decimal 0)
* Maximum: 11111111 (Decimal 255)

**Network ID vs Host ID**

Every IP address is split into two logical parts. No IP address stands alone; it always belongs to a group.
* The Network ID: This identifies the specific network (the "Street Name"). All devices on the same physical wire or Wi-Fi must have the same Network ID to talk to each other directly.
* The Host ID: This identifies the specific device (the "House Number"). This must be unique within that specific network.

**Critical Special Addresses**

Within every network, there are specific addresses that cannot be assigned to a computer because they have special jobs.

1. The Network Address (The "ID" of the Group)
    This is the very first address in a range. The Host ID bits are all set to 0.
    * Purpose: It identifies the network itself in a router's "map" (routing table).
    * Example: 192.168.1.0
    * Can you use it? No. You cannot assign this to a laptop.
    * The Network Address represents the entire group. If a specific device (like the router itself) took the Network Address as its personal IP, the system could not distinguish between "The whole network" and "That specific device."
    * The Rule: An address ending in all 0s (in its host portion) refers to the wire/segment, not a physical machine.

2. The Broadcast Address (The "Megaphone")
    This is the very last address in a range. The Host ID bits are all set to 1.
    * Purpose: If you send data to this address, every single device on that network receives it.
    * Example: 192.168.1.255
    * Can you use it? No. It is reserved for shouting to everyone.
3. 0.0.0.0 (The "Unknown" Address)
    Purpose: This is used by a device that doesn't have an IP address yet (like a computer just plugging into a network asking for an address). It means "This Host" or "Any Network."
4. 127.0.0.1 (The Loopback / "Self")
    Purpose: This is called localhost. It allows a computer to send data to itself. If you ping this address and get a reply, your computer’s networking software is working correctly.
5. 255.255.255.255 (The Limited Broadcast)
    Purpose: This is a "shout" that is restricted to the local wire only. Routers will never pass this message to another network.

6. The Gateway:
    * This is the IP address of the Router. When a device wants to send data to an IP that is not on its own network, it sends the data to the Gateway to be forwarded out to the internet.
    * Since the Network Address is reserved to identify the group, the Router needs its own unique Host Address to function.
        - Common Practice: Usually, the first usable address (e.g., .1) or the last usable address (e.g., .254) is assigned to the router.

Imagine a network where the first 3 octets are the Network ID and the last octet is the Host ID.

```
Network ID: 192.168.1.x

[192.168.1.0] <--- Network Address (Reserved)
      |
      +--- [192.168.1.1] (Router/Gateway)
      |
      +--- [192.168.1.2] (Laptop)
      |
      +--- [192.168.1.3] (Printer)
      |
[192.168.1.255] <--- Broadcast Address (Reserved)
```

### How IP route packet
The world uses a tiered system to prevent "Table Explosion" in routers.
* Tier 1 (Continental): A major router in New York might only have one rule: "If the address starts with 192 (North America), keep it here. If it starts with 45 (Europe), send it across the ocean cable."
* Tier 2 (National): A router in London sees an address starting with 45. It looks at the next part: "If it starts with 45.10, send it to the UK. If it starts with 45.20, send it to France."
* Tier 3 (Local ISP): A router in London sees 45.10. "This belongs to British Telecom (ISP). Send it to their exchange."
* Tier 4 (Firm/End User): The ISP sees the final part: "This belongs to the Coffee Shop at 123 Street."


*"If a business (e.g., a coffee shop) is assigned a specific IP address by an ISP at one location, but then physically relocates to a different city or country, can they retain and use that same IP address at the new location? If not, what happens to the original address?"*

If the Coffee Shop moves from London to Paris:
1. In London, their address was 45.10.x.x.
2. In Paris, the local routers are only looking for 45.20.x.x.
3. If the shop tries to use its London IP in Paris, the Paris routers will ignore it because it doesn't match the local "Prefix" rules.
4. Conclusion: The shop must get a new IP address from the Paris ISP. Their old IP is "waste" to them, but the London ISP will immediately give that IP to a new customer in London.

[Classificatio](./Classification.md)
[Packet Delivery](./classfull_packet_journey.md)