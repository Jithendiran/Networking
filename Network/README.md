## 1. The Data Link Layer and MAC Addresses
Every Network Interface Card (NIC) is manufactured with a unique identifier called a MAC Address (Media Access Control). This is a physical address. It allows devices to identify themselves on a local wire.

**Key Characteristics:**
* Uniqueness: No two devices globally should share the same MAC address.
* Scope: MAC addresses are only useful for communication within the same local network.
* Direct Delivery: When two devices are on the same switch, they use these addresses to exchange data.

## 2. Defining a Local Network (LAN)
A network is a collection of devices connected to a common layer-2 device, typically a Switch.
* Physical Connection: All devices in a single network share the same broadcast domain (switch).
* The Switch: A switch acts as a central hub. In a Star Topology, every PC has a dedicated cable connecting it to the switch.
* Multi-point vs Point-to-Point: 
    - The connection between multiple PCs and a switch is Multi-point.
    - A direct cable between a switch and a router is Point-to-Point.

## 3. Communication Within the Same Network
```
[ NETWORK A ]     
(Local MACs Only) 
                 
[PC-A1] [PC-A2]   
   |       |      
   └───┬───┘      
   [Switch A]
       |         
  (Direct Link)
```

When PC-A1 sends data to PC-A2, the process follows these steps:
1. Encapsulation: PC-A1 creates a "Frame." This frame contains the Source MAC (PC-A1) and the Destination MAC (PC-A2).
2. Switch Logic: Initially, if a switch does not know where PC-A2 is, it performs Flooding. It sends the frame out of every port except the one it arrived on.
3. Filtering: Every device on the switch receives the frame.
    * PC-A2 sees its own MAC address in the destination field and Accepts the frame.
    * Other devices see a mismatch and Drop the frame.
4. Learning: The switch remembers which port PC-A1 is on for future use.

### How a switch uses MAC addresses
A switch is a device that connects multiple machines inside a single local network. It keeps a table — called a CAM table or MAC address table — that maps each known MAC address to the physical port that device is connected to.

**How the table is built (learning):**
1. A frame arrives at the switch. The switch reads the source MAC address and records it: "this MAC is reachable through port 3." This is learning.
2. The switch checks if it knows the destination MAC. If it does, it sends the frame only to the correct port. This is forwarding.
3. If the switch does not know the destination MAC, it sends the frame out of every port except the one it arrived on. This is flooding.

Over time, as devices send traffic, the switch learns all MAC-to-port mappings and floods less frequently.

## 4. The Limitation of MAC Addresses

```
[ NETWORK A ]                               [ NETWORK B ]
(Local MACs Only)                           (Local MACs Only)
      |                                           |
[PC-A1] [PC-A2]                             [PC-C1] [PC-C2]
   |       |                                   |       |
   └───┬───┘                                   └───┬───┘
   [Switch A]----------[ ROUTER ]--------------[Switch B]
       |                    |                      |
  (Direct Link)       (The Boundary)          (Direct Link)
```

MAC addresses cannot be used to communicate between different networks for the following reasons:

### A. Physical Isolation
Networks are physically separated by Routers. A switch does not forward traffic to another switch unless there is a physical path. If PC-A1 sends a frame, that frame physically cannot "jump" to Switch B without a gateway.

### B. Scalability (The "Why")
If the entire world operated as one single network using only MAC addresses:
* Every switch would have to store billions of MAC addresses.
* Every time a device sent a search packet (Flooding), billions of devices would receive it, crashing the global internet.

### C. The Router's Role
A Router acts as a barrier and a bridge. It stops local "noise" (broadcasts) from leaving the network but allows specific data to pass through if it is destined for a different network.

**Why PC-A1 cannot reach PC-C2 with MAC alone**
1. Unknown Destination: PC-A1 has no way of "discovering" the MAC address of PC-C2 because discovery packets (like ARP) are blocked by the Router.
2. Hard-coding Failure: Even if the MAC address of PC-C2 is manually typed into PC-A1, the Switch A will look at the destination and realize that MAC address is not connected to any of its ports.
3. The Drop: Because the Switch does not see PC-C2 on any of its local cables, the packet has nowhere to go and is eventually dropped.

To communicate between Network A and Network B, a different type of addressing is required that sits "above" the MAC address. This is why the Network Layer (IP Addresses) exists.

## How new address should be?

### Why MAC break?
If the world attempted to use only MAC addresses for communication between different networks, two major failures would occur:
**1. Memory and Table Explosion**

In a local network, a switch remembers which port a MAC address is on. If PC-A1 wants to talk to PC-C1 (on a different network) using only MAC:
* The Requirement: Every switch in the world would need to store the location of every single device (all 4 billion+ devices).
* The Result: No switch has enough memory to hold a table of that size. The hardware would be overwhelmed trying to find a single destination among billions of entries.

**2. The Mobility/Update Crisis**

MAC addresses are static. If a device disconnects from Switch A and moves to Switch C:
* The Requirement: Every other switch in the world must be updated immediately to know that this specific MAC address has moved to a new path.
* The Result: The amount of "update data" traveling across the globe would consume all available bandwidth. The network would crash just trying to keep track of where devices are moving.

### Solving the Problem with Logical Hierarchies

To reduce this load, the system needs a Prefix-based approach.

**1. The Prefix Concept**

Imagine Switch A only cares about addresses starting with 1.1.x.x and Switch B only cares about 2.2.x.x.
* If a packet arrives at Switch A with a destination starting with 2.2, the switch does not need to know the specific device. It simply passes the packet to the next path leading toward Switch B.
* This keeps internal tables small because switches only store "Directions to Prefixes" rather than "Directions to Individuals."

**2. Why Manufacturers Cannot Solve This**

One might suggest that manufacturers should build devices with these prefixes (e.g., Apple makes AA devices, Dell makes BB devices).
* The Conflict: If a device with an AA prefix (manufactured for one location) moves to a network that only allows BB prefixes, the network would block it.
* The Conclusion: The prefix cannot be "burned into" the hardware by the manufacturer. It must be assigned by the network the device is currently visiting.

### Requirements for a New Addressing System (IP Design)
Before looking at the actual IP format, these are the characteristics engineers identified to solve the issues above:
1. Dynamic Assignment: 
    * When a device connects to a switch, the switch (or a controller) assigns it a temporary address.
    * When the device disconnects, that address is freed and can be reused for the next device.
2. Hierarchical Network Addresses:
    * The address must contain a Network Portion (the prefix) and a Host Portion (the specific device).
    * Other switches only need to know how to reach the "Network" part. They do not need to know about the thousands of devices inside that network.
3. MAC as the Foundation:
    * Even with a new dynamic address, the MAC address is still required.
    * The "Why": When a device first connects, it has no dynamic address yet. The switch uses the MAC address as a hardware identifier to talk to the device and give it its new logical address.

#### MAC can't ran out?
MAC addresses use 48 bits.
* The Calculation: $2^{48}$
* The Total: Approximately 281 Trillion addresses. 

Key Fact: If every person on Earth owned 35,000 smart devices, the world would still have enough MAC addresses to give every single device a unique ID. Estimated to last until 2100+, so no one focus now, when we at that time, we may have completely different technology
