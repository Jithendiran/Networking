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

## 4. The Limitation of MAC Addresses

```
[ NETWORK A ]                               [ NETWORK B ]
(Local MACs Only)                           (Local MACs Only)
      |                                           |
[PC-A1] [PC-A2]                             [PC-C1] [PC-C2]
   |       |                                   |       |
   └───┬───┘                                   └───┬───┘
   [Switch A]----------[ ROUTER ]-----------[Switch B]
       |                    |                    |
  (Direct Link)       (The Boundary)        (Direct Link)
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


How should be IP address looks like? how it's characteristics 

before seeing IP, let's see at the time of no IP address what are the characteristics engineers looked for and designed it