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

#### 3. Host Capacity Logic
Each Class A network uses $32-8\text{(network)}=24$, 24 bits for host addresses. 

* **Total Mathematical Addresses:** $2^{24} = 16,777,216$
* **Usable Hosts per Network:** $16,777,216 - 2 = 16,777,214$.
* **The "Minus 2" Requirement:** Every network requires two addresses for management, which cannot be assigned to devices:
    1.  **Network Address:** All host bits are set to **0**. (Example: `10.0.0.0`). This identifies the network itself.
    2.  **Broadcast Address:** All host bits are set to **1**. (Example: `10.255.255.255`). This is used to send data to every device on that specific network simultaneously.


#### 4. Broadcast Address
A common error is assuming `127.255.255.255` is the only broadcast address. Every individual network has its own unique broadcast address.

* If the network is `1.0.0.0`, the broadcast is `1.255.255.255`.
* If the network is `10.0.0.0`, the broadcast is `10.255.255.255`.
* **Note:**In Class A, the broadcast only occurs when **all 24 host bits** (the last three octets) are 255.

## Todo
All class
drawback of class
classless