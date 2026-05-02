# QEMU Isolated Environment Setup: Alpine Linux

## 1. Environment Preparation
Before executing virtualization commands, a dedicated workspace ensures that temporary files do not interfere with the host system's primary directories.

*   **Directory:** `/tmp/networking`
*   **Purpose:** The `/tmp` directory is used for volatile data that does not need to persist after a system reboot. Creating a sub-directory organizes all related disk images and configuration files in one location.


## 2. Resource Acquisition
Virtualization requires an **ISO image**, which is a single file containing all the data found on an optical disc (CD or DVD). 

*   **Command:** `wget https://dl-cdn.alpinelinux.org/alpine/latest-stable/releases/x86_64/alpine-virt-3.23.4-x86_64.iso`
*   **Logic:** **Alpine Linux (Virt edition)** is selected because it is a "lightweight" distribution. The **Virt** flavor is specifically stripped of hardware drivers unnecessary for virtual machines, resulting in faster boot times and lower memory consumption.


## 3. The Execution Command
The following command initializes the **QEMU (Quick Emulator)** software to create a virtual hardware environment.

```bash
qemu-system-x86_64 \
  -enable-kvm \
  -m 512 \
  -nic none \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -boot d \
  -nographic
```

### 3.1 Technical Parameter Definitions
| Switch | Logic and Purpose |
| :--- | :--- |
| **`-enable-kvm`** | **Kernel-based Virtual Machine.** This allows the guest VM to use the host's CPU features directly. Without it, the host must emulate every CPU instruction via software, which is significantly slower. |
| **`-m 512`** | **Memory Allocation.** Assigns 512 Megabytes of RAM. This provides a balance between host performance and guest stability for complex networking tasks. |
| **`-nic none`** | **Network Interface Controller.** Modern QEMU defaults to creating a virtual network bridge. Setting this to `none` ensures the guest has no path to communicate with the host or the internet. |
| **`-drive ...,media=cdrom`** | **Storage Mapping.** Defines the downloaded ISO file as a virtual CD-ROM drive. |
| **`-boot d`** | **Boot Order.** Instructs the virtual BIOS to boot from the "D" drive (CD-ROM) instead of a hard disk. |
| **`-nographic`** | **Display Output.** Redirects the VM output to the current terminal window instead of opening a separate graphical window. |


## 4. Guest Environment Verification
Once the system boots, the user logs in as **root** with no password. To verify the isolated state, the following command is used:

*   **Command:** `ip link`
*   **Observed Result:** Only the **lo** (loopback) interface is visible.
*   **Underlying Logic:** The **loopback interface** is a virtual interface used by the OS to talk to itself. The absence of interfaces like `eth0` or `ens3` confirms that the `-nic none` flag successfully prevented the creation of external communication hardware.


## 5. Termination Procedure
Because the environment is running in `-nographic` mode, standard window close buttons are unavailable. The process must be terminated via the QEMU monitor escape sequence.

1.  **Sequence:** Press `Ctrl + a`.
2.  **Action:** Release keys and press `x`.
3.  **Logic:** This sequence signals the QEMU process to "Exit" and terminate the virtual machine immediately, returning control to the host terminal.


# QEMU Networking Modes
This documentation outlines the methods available for connecting Virtual Machines (VMs) in a QEMU environment. These methods are categorized by their complexity and their function within the OSI Model (the standard for how computers communicate).

## 1. User-mode Networking (SLIRP)
**Logical Role:** The "Virtual Home Router."

*   **Logic:** QEMU creates a built-in, isolated network stack that acts as a DHCP server and a NAT (Network Address Translation) gateway. The VM is assigned a private address (typically `10.0.2.15`).
*   **Purpose:** This is the default mode. It allows the VM to reach the internet (to download packages or browse web pages) without requiring administrative privileges on the host.
*   **Constraint:** The VM is invisible to the host and other VMs. External traffic cannot reach the VM unless specific port forwarding is configured.


## 2. Multicast Sockets
**Logical Role:** The "Virtual Hub."

*   **Logic:** Multiple QEMU instances are configured to send and receive data on a specific **Multicast IP and Port**. 
*   **Purpose:** Any VM using the same address/port combination "hears" the traffic of the others, effectively simulating a shared Ethernet hub. 
*   **Advantage:** It requires no administrative (`root`) permissions on the host. It is the most efficient way to build a private internal laboratory where VMs must communicate only with each other.


## 3. TAP Interfaces and Linux Bridges
**Logical Role:** The "Physical Patch Cable."

*   **Logic:** A **TAP** is a software-based Ethernet device. It acts as a bridge between the VM's virtual network card and the host's kernel. The TAP is usually connected to a **Linux Bridge** (a software-defined switch).
*   **Purpose:** This method integrates the VM into a network as if it were a physical machine. If the Bridge is connected to a physical network card, the VM can receive an IP address from a real-world router.
*   **Constraint:** Requires `sudo` or `root` privileges to create and manage the virtual interfaces on the host system.



## 4. VDE (Virtual Distributed Ethernet)
**Logical Role:** The "Managed Enterprise Switch."

*   **Logic:** A separate background process (`vde_switch`) runs on the host. QEMU instances connect to this process using a virtual "plug."
*   **Purpose:** VDE supports advanced features like VLANs (Virtual Local Area Networks) and spanning tree protocols. It is used for simulating large-scale, complex corporate networks.
*   **Constraint:** Requires the installation of external VDE software components on the host.


## 5. Socket (Listen/Connect)
**Logical Role:** The "Point-to-Point Cable."

*   **Logic:** One QEMU instance is set to "Listen" on a specific TCP/UDP port, while a second QEMU instance is set to "Connect" to that same port.
*   **Purpose:** This simulates a single, direct crossover cable between two specific devices. It is used for dedicated links between two routers where no other devices should interfere.


## 6. Additional Advanced Modes
For the sake of exhaustive documentation, the following high-performance or specialized modes also exist:

### 6.1. Vhost-net
*   **Logic:** This is an optimization for **TAP** networking. It moves the packet-processing tasks from the QEMU software directly into the Linux Kernel.
*   **Purpose:** It reduces "overhead" (wasted CPU cycles), allowing for much higher data speeds between the VM and the network.

### 6.2. IVSHMEM (Inter-VM Shared Memory)
*   **Logic:** VMs share a specific region of the host’s physical RAM to pass data back and forth.
*   **Purpose:** This is the fastest possible communication method. It bypasses standard networking protocols entirely to achieve near-instant data transfer for specialized high-speed applications.

### 6.3. Passthrough (SR-IOV)
*   **Logic:** The host gives the VM direct, exclusive control over a physical network hardware slice.
*   **Purpose:** Used when the VM requires "Bare Metal" performance with zero virtualization delays. This requires specific hardware support in the host CPU and network card.

## Optimal QEMU Networking for Lab Environments

When constructing a multi-node networking laboratory, the selection of a networking mode depends on the specific educational objective: **Internal Isolation**, **External Connectivity**, or **Scalability**.


### 1. Multicast Sockets: The Best for Internal Logic
**Multicast Sockets** are the primary recommendation for a beginner’s internal lab. This mode acts as a "Virtual Hub."

*   **Underlying Logic:** QEMU sends Ethernet frames via UDP multicast packets to a specific IP address and port (usually `230.0.0.1:1234`). Any VM "listening" on that same socket receives the frame.
*   **Purpose:** To simulate a shared physical wire between multiple devices without modifying the host system.
*   **Why it is best for labs:**
    *   **No Root Privileges:** Unlike Bridges or TAPs, any user can launch these VMs.
    *   **Simplicity:** No complex configuration files or background daemons are required on the host.
    *   **Isolation:** The traffic remains strictly within the defined multicast group, preventing interference with the host's real network.
*   **Command Syntax Example:**
    `-netdev socket,id=n1,mcast=230.0.0.1:1234 -device virtio-net-pci,netdev=n1`

### 2. User-mode (SLIRP): The Best for Simple Internet Access
**User-mode Networking** is the optimal choice when a VM requires internet access for package updates (e.g., `apk add` in Alpine) but does not need to communicate with other VMs.

*   **Underlying Logic:** QEMU acts as a tiny NAT router and DHCP server. It translates the VM's internal requests into standard system calls on the host.
*   **Purpose:** To provide a "black box" internet connection.
*   **Why it is best for labs:**
    *   **Zero Configuration:** It is the default state if no network flags are provided.
    *   **Safety:** The VM is behind a one-way firewall; the host can reach the internet, but the internet cannot reach the VM.
*   **Command Syntax Example:**
    `-netdev user,id=n1 -device virtio-net-pci,netdev=n1`


### 3. TAP Interfaces and Linux Bridges: The Best for Real-World Simulation
**TAP/Bridge** networking is the standard for labs that must mimic a professional data center or enterprise environment.

*   **Underlying Logic:** A **TAP** device is a virtual Ethernet card on the host. A **Bridge** is a virtual switch. Connecting the TAP to the Bridge effectively "plugs" the VM into a switch.
*   **Purpose:** To allow the VM to appear as a peer device on the physical network.(Host system act as a bridge, VM can directly access gateway, gateway will assign IP to it, it is like a peer computer)
*   **Why it is best for labs:**
    *   **External Reachability:** Other physical computers on the local network can ping and SSH into the VM.
    *   **Performance:** It offers higher throughput and lower latency than Multicast or SLIRP.
*   **Constraint:** This requires `sudo` privileges to create the bridge (`br0`) and the tap interface (`tap0`) on the host.

#### 3.1 Configuration: TAP Interfaces and Linux Bridges

Implementing a TAP and Bridge setup requires two distinct phases: configuring the **Host infrastructure** (creating the virtual switch and the link) and launching the **QEMU Guest** (attaching the virtual network card to that link).

##### Phase 1: Host Infrastructure Setup
These commands must be executed on the Linux host with `sudo` or `root` privileges. They create a persistent environment where the VM can reside.


1.  **Create the Bridge (The Virtual Switch):**
    `ip link add name br0 type bridge`
    *   **Logic:** This initializes a software-defined Layer 2 switch inside the Linux kernel.

2.  **Create the TAP Interface (The Virtual Port):**
    `ip tuntap add dev tap0 mode tap`
    *   **Logic:** A TAP interface operates at the Data Link layer (Ethernet). It acts as the bridge's "port" where the VM will plug in.

3.  **Link the TAP to the Bridge:**
    `ip link set tap0 master br0`
    *   **Logic:** This physically "wires" the TAP port into the `br0` switch.

4.  **Activate the Interfaces:**
    `ip link set br0 up`
    *   `ip link set tap0 up`
    *   **Logic:** Interfaces are created in a "DOWN" state by default. They must be administratively enabled to pass traffic.

##### Phase 2: QEMU Execution
Once the host infrastructure is active, QEMU must be instructed to use the specific TAP device created.

*   **Command:**
```bash
qemu-system-x86_64 \
  -enable-kvm \
  -m 512 \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -netdev tap,id=n1,ifname=tap0,script=no,downscript=no \
  -device virtio-net-pci,netdev=n1 \
  -nographic
```

#### Technical Parameter Definitions (Networking)

| Parameter | Logic and Purpose |
| :--- | :--- |
| **`-netdev tap`** | Defines the backend type. Unlike `user` (SLIRP), this tells QEMU to look for a physical or virtual device on the host. |
| **`id=n1`** | A unique internal label for this network backend, used to link it to the virtual hardware. |
| **`ifname=tap0`** | Specifies the exact name of the TAP interface created in Phase 1. |
| **`script=no`** | Disables the default QEMU "up" script. Since the bridge and tap were configured manually, QEMU does not need to attempt automated configuration. |
| **`-device virtio-net-pci`** | Defines the virtual hardware the Guest OS sees. **Virtio** is used because it is a paravirtualized driver, offering significantly higher speed than emulated cards like `e1000`. |
| **`netdev=n1`** | Connects the virtual hardware (`virtio-net-pci`) to the network backend (`n1`). |


##### Phase 3: Verification (Inside the Guest)
After the Alpine VM boots and the user logs in as `root`, the interface must be configured to communicate.

1.  **Command:** `ip addr add 192.168.1.50/24 dev eth0`
2.  **Command:** `ip link set eth0 up`
3.  **Logic:** The Guest OS sees the Virtio device as `eth0`. By assigning an IP address within the same subnet as the host's bridge (`br0`), a direct communication path is established.


#### Cleanup Procedure
To remove the laboratory infrastructure from the host after the session:
1.  **Delete the TAP:** `ip link delete tap0`
2.  **Delete the Bridge:** `ip link delete br0`
*   **Logic:** Deleting the interfaces ensures no residual virtual hardware consumes system resources or creates network loops on the host.


### 4. Summary Selection Table

| Lab Goal | Recommended Mode | Logic / Why |
| :--- | :--- | :--- |
| **Learning Protocol Analysis** | **Multicast Sockets** | Simulates a hub; allows multiple VMs to "hear" each other's traffic for tools like Wireshark. |
| **Installing Software** | **User-mode (SLIRP)** | Easiest way to get the VM online to download tools without host configuration. |
| **Server Hosting Lab** | **TAP/Bridge** | Allows the host or external devices to access services (Web, DNS, DHCP) running inside the VM. |
| **Complex Topologies** | **VDE** | Best for simulating VLANs and managed switch behavior across many nodes. |


### 5. Recommended Lab Workflow
For an optimal Alpine Linux networking lab, a **dual-interface** approach is frequently employed:

1.  **Interface 1 (User-mode):** Provides the VM with internet access to download networking tools (`iproute2`, `tcpdump`, `iperf`).
2.  **Interface 2 (Multicast Socket):** Connects the VM to a private "internal-only" wire for experimenting with routing and packet capture without external noise.

**Example Command for Dual-Interface Lab:**
```bash
qemu-system-x86_64 \
  -enable-kvm -m 512 \
  -drive file=alpine.iso,media=cdrom \
  -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \ # for network
  -netdev socket,id=net1,mcast=230.0.0.1:1234 -device virtio-net-pci,netdev=net1 \ # for testing
  -nographic
```

----------------------------------------------------------

# Setup

## Layer 2 Data Link Laboratory

This document serves as a permanent reference for establishing and verifying Layer 2 connectivity between two isolated virtual nodes using QEMU Multicast Networking.

## 1. Laboratory Topology: The Virtual Hub
In this configuration, QEMU utilizes a **Multicast Socket** to simulate a physical Ethernet hub. Every packet sent by one VM is received by all other VMs configured with the same multicast IP and port.

*   **Multicast Address:** `230.0.0.1` (The "Shared Wire")
*   **Port:** `1234` (The "Channel")
*   **Logic:** This simulates a broadcast domain where nodes communicate via MAC addresses without host-level intervention.


## 2. Phase 1: Virtual Hardware Initialization
The following commands initialize the virtual machines with unique hardware identities (MAC addresses).

### Node A Initialization
```bash
qemu-system-x86_64 -enable-kvm -m 512 \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -netdev socket,id=n1,mcast=230.0.0.1:1234 \
  -device virtio-net-pci,netdev=n1,mac=52:54:00:12:34:01 \
  -nographic
```

### Node B Initialization
```bash
qemu-system-x86_64 -enable-kvm -m 512 \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -netdev socket,id=n1,mcast=230.0.0.1:1234 \
  -device virtio-net-pci,netdev=n1,mac=52:54:00:12:34:02 \
  -nographic
```

## 3. Phase 2: Administrative Activation
By default, the Linux kernel boots with network interfaces in a **DOWN** state. No data can be processed until the interface is administratively enabled.

1.  **Command:** `ip link set eth0 up`
2.  **Verification:** `ip link show eth0`
3.  **Logic:** The appearance of the `<UP,LOWER_UP>` flags indicates that the software driver is active (`UP`) and the virtual physical carrier is detected (`LOWER_UP`).

## 4. Phase 3: Address Resolution (ARP)
Nodes on an Ethernet network cannot communicate using IP addresses directly; they must resolve IPs into **MAC Addresses**. This is handled by the **Address Resolution Protocol (ARP)**.


### Step A: Assign Logical IPs
Assigning IPs allows the nodes to identify each other within the software stack.
*   **Node A:** `ip addr add 192.168.1.1/24 dev eth0`
*   **Node B:** `ip addr add 192.168.1.2/24 dev eth0`

### Step B: Trigger the ARP Event
Initially, the **ARP Cache** (Neighbor Table) is empty. Running a `ping` forces the nodes to discover one another.
*   **Action:** `ping -c 2 192.168.1.1` (from Node B)

### Step C: Verify the Neighbor Table
*   **Command:** `ip neigh show`
*   **Logic:** If the ping is successful, Node B stores Node A's MAC address in its memory to avoid broadcasting for every future packet.
*   **Expected Output:** `192.168.1.1 dev eth0 lladdr 52:54:00:12:34:01 REACHABLE`