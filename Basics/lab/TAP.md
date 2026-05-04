## Phase 1: The Virtual Network Interface (TAP)

### 1. Definition: What is a TAP Device?
A TAP (Network Tap) is a software-only virtual network interface provided by the Linux kernel. It operates at Layer 2 (Data Link Layer) of the OSI model.

* **Logic**: In a standard setup, a physical network card (NIC) receives electrical signals and converts them into Ethernet frames for the kernel to process. A TAP device replaces the hardware. It acts as a "virtual wire" where one end is plugged into the Linux kernel and the other end is plugged into a user-space program.
    > In standard networking, a Network Interface Card (NIC) is a physical piece of hardware (like an Ethernet port). The operating system kernel manages this hardware through a driver. User-space programs (like web browsers) do not talk to the hardware directly; they send data to the kernel, which then pushes it out through the physical wire.
* **The Function:** A TAP device acts as a bridge. When the kernel "sends" data to the TAP interface, the data is redirected to a file descriptor held by the user-space program. When the program "writes" data to that file descriptor, the kernel treats it as if a physical Ethernet cable just delivered a frame to a hardware NIC.

### 2. Purpose: Why is it used?
The primary reason for using a TAP device in protocol development is *Isolation* and *Control*.
* **Bypassing the Kernel**: Normally, the Linux kernel handles all incoming packets (ARP, IP, TCP). By using a TAP device, the kernel hands the raw Ethernet frame directly to the user-space program.
* **Userspace Development**: It allows the developer to write and test networking code as a standard application without the risk of crashing the entire operating system (which happens if one makes a mistake in kernel-level code).
* **Hardware Independence**: A developer can build an entire networking stack on a laptop without needing physical routers, switches, or specialized cables.

### 3. Configuration & Options
The TAP device is managed through a "Clone Device" located at /dev/net/tun. To configure it, a program uses the ioctl() (Input/Output Control) system call with the struct ifreq structure.

#### The Concept of a Clone Device
A Clone Device is a specific type of software architecture used by the Linux kernel to manage virtual resources. Instead of the system having thousands of static files representing potential virtual interfaces, it provides one single entry point: `/dev/net/tun`.

**Why the Clone Model Exists:**
* Resource Efficiency: It prevents the filesystem from being cluttered with unused device nodes.
* Dynamic Allocation: It allows interfaces to be created and destroyed instantly based on the needs of active software.

**The Structural Components**

To facilitate the cloning process, two primary structures are used:
* The Device File (`/dev/net/tun`): This is a character device file. It serves as the "factory" or "template" from which specific interfaces are generated.
* The ifreq Structure: A standard data structure (Interface Request) used to pass configuration details between a program and the kernel. It contains the name of the desired interface (e.g., `eth0` or `tap0`) and flags to specify the type (`TUN` or `TAP`).

**The Step-by-Step Cloning Process**
1. Phase 1: Obtaining the File Descriptor
    The program performs a standard `open()` system call on `/dev/net/tun`. At this stage, the resulting File Descriptor (FD) is generic. It is a handle to the "factory," but it is not yet associated with any network traffic.

2. Phase 2: The ioctl (Input/Output Control) Request
    The program must tell the kernel what it wants to create. It uses the `ioctl()` system call, which is a method for hardware-specific or driver-specific communication.
    * The Command: `TUNSETIFF` (Tun Set Interface).
    * The Data: The `ifreq` structure containing the interface name and mode.

3. Phase 3: The Kernel Allocation
    Upon receiving the `TUNSETIFF` command, the kernel searches for an existing interface with that name.
    * If the name is available, the kernel allocates a new virtual network device in memory.
    * The kernel then "links" or "clones" the driver's logic specifically to the File Descriptor held by the program.

4. Phase 4: Data Flow Establishment
    The File Descriptor is now unique. It no longer points to the generic factory; it points to the specific instance of the virtual interface.
    * Write Operation: If the program writes data to the FD, the kernel injects that data into the network stack as if it were an incoming packet from a physical wire.
    * Read Operation: If the operating system sends a packet out through the virtual interface, that data becomes available for the program to read from the FD.

**Persistence and Cleanup**
The "cloned" interface is inherently tied to the lifecycle of the File Descriptor. If the program closes the FD or the program terminates, the kernel recognizes that the "wire" has been unplugged. The kernel then automatically destroys the virtual interface and frees the associated memory. This prevents "zombie" interfaces from remaining on the system after a program finishes execution.

#### Config

* `IFF_TAP`: Specifies that the device should be a TAP (Layer 2 - Ethernet frames).
    - Logic: If `IFF_TUN` were used instead, the device would operate at Layer 3 (IP packets only), stripping away the Ethernet headers.
* `IFF_NO_PI`: "No Packet Information."
    - Logic: By default, the kernel prepends 4 bytes of metadata (flags and protocol) to every packet. For manual protocol development, these 4 bytes interfere with the standard 14-byte Ethernet header. Setting `IFF_NO_PI` ensures the program receives a "clean" Ethernet frame.
* `IFNAMSIZ`: A constant defining the maximum length of the interface name (e.g., "tap0").
* `TUNSETIFF`: When the file `/dev/net/tun` is opened, it returns a generic file descriptor that is not yet associated with any network interface. The `ioctl(fd, TUNSETIFF, ...)` call tells the kernel: "Take this generic file descriptor and attach it to a specific network interface (TAP or TUN) with these specific properties."

### 4. Implementation

#### c

```c
struct ifreq ifr;
int fd = open("/dev/net/tun", O_RDWR);

memset(&ifr, 0, sizeof(ifr));
ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
strncpy(ifr.ifr_name, "tap0", IFNAMSIZ);

ioctl(fd, TUNSETIFF, (void *)&ifr);

...
read(fd, buffer, sizeof(buffer));
write(fd, buffer, length);
```
* `read(fd, buffer, sizeof(buffer))`: This call blocks until a frame arrives. When the kernel sends a frame to `tap0`, the raw bytes (starting with the Destination MAC address) are copied into the buffer.
* `write(fd, buffer, length)`: This call injects bytes into the network. The kernel treats the bytes in buffer as a frame arriving from a physical wire.

#### terminal
To simulate the `TUNSETIFF` and `ifr_flags` configuration from the terminal, use the ip tuntap command.
* Command: `sudo ip tuntap add mode tap name tap0`
    - `add`: Tells the kernel to allocate a new interface.
    - `mode tap`: Corresponds to the `IFF_TAP` flag in C (Layer 2).
    - `name tap0`: Corresponds to `ifr.ifr_name`.
* This creates the persistent "device" in the kernel. Unlike the C code (which usually destroys the interface when the program closes), a TAP device created via terminal remains until explicitly deleted.

Deleting the Interface

* Command: `sudo ip link delete tap0` 
    - This performs the cleanup, removing the virtual device from the kernel's networking stack.

#### IP setup
This is typically done outside of the C program using terminal commands, as the driver's job is simply to provide the data link.

To send data to your user-space program, the kernel needs to know which IP addresses belong to that "link."

* Command: `sudo ip addr add 192.168.1.1/24 dev tap0`
    - Logic: This tells the Linux Network Stack: "Your address on the `tap0` network is `192.168.1.1`."


### 5. Debugging and Troubleshooting
1. Step 1: Verification of Existence
    Command: `ip link show`
    * Logic: If the interface `tap0` does not appear, the `ioctl()` call failed (likely due to lack of `sudo` permissions).
    ```
    3: tap0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
    link/ether aa:bb:cc:dd:ee:ff brd ff:ff:ff:ff:ff:ff
    index: name: <flag> property <value> ....
    ```
    * index: `3`. The unique kernel index number for the interface.
    * Name: `tap0`. The name of the interface.
    * Flags: `<BROADCAST,MULTICAST,UP,LOWER_UP>`. 
        - `BROADCAST`: the interface type supports sending one frame to all devices simultaneously. Ethernet is a broadcast medium by design. TAP devices inherit this because they emulate Ethernet. A pure TUN device would not carry this flag.
        - `MULTICAST`: the interface can address a subset of devices (a multicast group). This is required for protocols like IPv6 Neighbor Discovery and some routing protocols. Ethernet (and TAP) has this because the hardware address space reserves a range for group addresses.
        - `UP`:  this is the administrative state. A human (or a program with CAP_NET_ADMIN) explicitly turned the interface on. Setting this flag does not guarantee traffic will flow; it only signals intent. It is set by `ip link set tap0 up`. Without this flag the kernel will not route packets to the interface.
        - `LOWER_UP` — this is the physical link state. For a hardware NIC it means a cable is plugged in and a signal is detected. For a TAP device it means a user-space program currently holds the file descriptor open. When the C program calls  `open("/dev/net/tun")` and completes `ioctl(TUNSETIFF)`, the kernel sets `LOWER_UP`. When the program closes or crashes, the kernel clears it.
        - `NO-CARRIER` (appears instead of `LOWER_UP` when missing) — the interface is administratively `UP` but has no link signal. For `TAP`: the interface was created with `ip tuntap add`, has been set up, but no C program has the fd open. This is the most common confusion point early in development.
            - The typical developer error is: creating the interface and setting it UP from the terminal, then wondering why `ip link show` says `NO-CARRIER`. The answer is that the C program has not yet opened the fd. The kernel has no "wire" to plug into.

        ```
        1. sudo ip tuntap add mode tap name tap0      ← interface exists, DOWN, NO-CARRIER
        2. sudo ip link set tap0 up                   ← UP but still NO-CARRIER
        3. [C program opens /dev/net/tun + ioctl]     ← NOW: UP + LOWER_UP
        4. sudo ip addr add 192.168.1.1/24 dev tap0   ← kernel can now route to it
        ```
    * `mtu 1500`:  Maximum Transmission Unit. The largest Ethernet payload that can be sent in one frame. 1500 bytes is the standard for Ethernet. The 14-byte Ethernet header is not counted in the MTU. If a packet is larger, the IP layer fragments it. TAP devices default to 1500 to match real Ethernet.
    * `qdisc pfifo_fast`: Queuing Discipline. The kernel queues outgoing packets before transmitting. `pfifo_fast` is a simple three-band priority queue.
    * `group default`: interface group for bulk management (`ip link set group default up`)
    * `qlen 1000`: the transmit queue length (number of packets). If the interface is slow and 1000 packets queue up, subsequent packets are dropped. At 1500 bytes per packet this is ~1.5 MB of buffered data.
    * `link/ether aa:bb:cc:dd:ee:ff`: the Layer 2 address type is Ethernet, and the MAC address is aa:bb:cc:dd:ee:ff
    * `brd ff:ff:ff:ff:ff:ff`:  the broadcast address for this link.
2. Step 2: Verification of State
    Command: `sudo ip link set tap0 up`
    * Logic: A TAP device is created in a "DOWN" state. No data will be sent or received until the interface is administratively enabled.
3. Step 3: Traffic Observation (The "Gold Standard")
    Command: `sudo tcpdump -ni tap0 -e`
    * Logic: The `-e` flag tells tcpdump to show the Ethernet headers. If the program writes a packet but tcpdump sees nothing, the error is in the C code's `write()` logic. If tcpdump sees the packet but it is "Malformed," the error is in the byte-alignment or `struct` packing.
4. Step 4: Common Failure - "Permission Denied"
    * Logic: Opening `/dev/net/tun` requires `CAP_NET_ADMIN` capabilities.
    * Correction: Always run the compiled networking binary with `sudo`.

### 5. Bridge
A Linux bridge is a software Layer 2 switch. It connects multiple network interfaces so that frames arriving on any port are forwarded to the correct port based on MAC address learning, exactly like a hardware Ethernet switch.

Without a bridge, each interface is isolated. With a bridge:
```
[tap0]  [tap1]  [eth0]
   \       |      /
    \      |     /
    [  br0 bridge  ]
```
Frames arriving on `tap0` can reach `eth0` and vice versa. The bridge learns which MACs are on which port by inspecting source MAC addresses of passing frames. This is identical to how a physical switch operates.
Using Bridges, virtual machine nodes can be exposed to outside internet as a separate machine, it recieve unique IP from the DHCP as the host computer receives

- `eth0` is the Host machine's actual NIC card
- `tap0` is the Virtual NIC card 
- `br0` is the virtual bridge 


```bash
sudo ip link add name br0 type bridge     # create the bridge
sudo ip link set tap0 master br0          # attach tap0 to bridge
sudo ip link set eth0 master br0          # attach eth0 to bridge
sudo ip link set br0 up                   # bring bridge up
```

`ip link set tap0 master br0` - meaning `tap0` is attached to `br0`, `br0` is the master meaning it owns the MAC and port routing table it decides which packet has to move where

`bridge link show` use this command to see bridges and it's connected devices

### 6. Verification: The "Gold Standard" Test
Once the TAP device is initialized and "UP," it can be verified without writing protocol code:

1.  **Run the program:** The program should sit in a loop calling `read()` on the TAP file descriptor.
2.  **Generate Traffic:** In a separate terminal, use `ping -I tap0 192.168.1.1` (using a dummy IP).
3.  **Observe Hexdump:** The program should print the raw bytes of the `ARP` or `ICMP` requests generated by the Linux kernel. If the first 6 bytes match the destination MAC and the next 6 match the source, the driver is functioning correctly.

### 7. Cleanup
Bridges and TAP devices are presistence so manual cleann up required
```bash
sudo ip link delete tap0
sudo ip link delete br0
``` 