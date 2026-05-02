# Network Troubleshooting: Bottom-Up Methodology

The rule: A layer is only trusted after the layer below it has been verified. Upper-layer tests run on a broken lower layer are meaningless.

## Layer 1: The Physical Laye

The Physical Layer governs the electrical, optical, and mechanical signals that move bits between devices. If Layer 1 is not functional, the software-defined layers above it (IP, TCP, HTTP) cannot communicate.

### 1. Diagnostics: Identifying the Failure
Verification must follow a logical sequence from the hardware to the operating system driver.

| Component | Symptom | Diagnostic Command |
| :--- | :--- | :--- |
| **Interface Visibility** | The NIC is not recognized by the OS. | `lspci` (PCI devices) or `lsusb` (USB adapters). |
| **Driver Status** | The hardware exists but no `eth0` or `tap0` is created. | `dmesg \| grep -i eth` or `lsmod` to check for loaded modules. |
| **Physical Link** | No light on the NIC; cable may be unplugged. | `ip link show` (Look for `NO-CARRIER`). |
| **Link Integrity** | Connection exists but is unstable or slow. | `ethtool <interface>` (Check Speed/Duplex). |
| **Hardware Errors** | Packet loss occurring at the wire level. | `ethtool -S <interface>` (Check `rx_crc_errors` or `drop_counters`). |

### 2. Physical Layer Troubleshooting Flow

#### Step A: Verify Hardware Recognition
Before checking for a "link," ensure the Operating System sees the Network Interface Card (NIC).
*   **Command:** `lspci | grep -i network`
*   **Logic:** If the hardware does not appear here, the card is either not seated correctly in the slot, lacks power, or has suffered a total hardware failure.

#### Step B: Verify Driver Initialization
The kernel must load a driver to translate hardware signals into data the OS can use.
*   **Command:** `dmesg | grep -i "eth"\|"net"`
*   **Logic:** Look for "probed" or "initialized." If the hardware is seen in `lspci` but not in `ip link`, the driver (kernel module) is missing or crashed.

#### Step C: Verify Link Carrier (The Cable)
This confirms an electrical circuit is completed between two devices (e.g., your computer and a switch).
*   **Command:** `ip link show <interface>`
*   **Logic:** 
    *   `state DOWN`: The interface is software-disabled (Admin down).
    *   `state UP` but `NO-CARRIER`: The interface is on, but the cable is unplugged, the other end is off, or the wire is cut.

#### Step D: Verify Negotiation (Speed/Duplex)
Both ends of a cable must agree on how fast to talk.
*   **Command:** `ethtool <interface>`
*   **Logic:** Look for "Link state", "Speed" and "Duplex." If one side is "Half-Duplex" and the other is "Full-Duplex," collisions will occur, causing massive packet loss despite having a "Link."


### 3. Fixing Layer 1 Issues

Once the specific failure point is identified, apply the corresponding fix:

*   **For `NO-CARRIER`:** 
    1. Re-seat the cable at both ends. 
    2. Swap the cable with a known working one.
    3. Change the port on the switch/router (the port itself may be dead).
*   **For `state DOWN`:**
    *   **Fix:** `sudo ip link set <interface> up`.
*   **For Driver Failures:**
    *   **Fix:** Identify the module name and force-load it: `sudo modprobe <module_name>`.
*   **For Speed/Duplex Mismatch:**
    *   **Fix:** Force auto-negotiation: `sudo ethtool -s <interface> autoneg on`.
*   **For CRC/Input Errors:**
    *   **Fix:** This usually indicates electromagnetic interference or a damaged copper wire. Replace the cable and move it away from high-voltage power lines.

In the **Bottom-Up** approach, Layer 1 is the "Ground." If `ethtool` shows `Link detected: no`, it is a waste of time to check IP addresses (`ip addr`) or test connectivity (`ping`). The fix must be physical (hardware/cable) or driver-related before moving to Layer 2.

## Layer 2: The Data Link Layer (Frames and Physical Addressing)
The Data Link layer is responsible for the transfer of data between two nodes connected to the same physical network segment. It packages data from the Network Layer into Frames and ensures they reach the correct local destination using MAC Addresses.

If Layer 1 is "UP" but communication fails, the issue usually resides in addressing or VLAN (Virtual LAN) tagging.

### 1. Diagnostics: Identifying Layer 2 Failures
If Layer 1 is "UP" but communication fails, the issue usually resides in addressing or VLAN (Virtual LAN) tagging.

| Symptom | Probable Cause | Diagnostic Command |
| :--- | :--- | :--- |
| **Incomplete ARP** | The target IP is on the local network, but its MAC cannot be found. | `ip neigh` or `arp -n` |
| **VLAN Mismatch** | The NIC is sending untagged frames, but the switch expects a VLAN tag. | `bridge link show` |
| **Duplicated MAC** | Two devices have the same MAC, causing "flapping" connections. | `dmesg | grep "duplicate address"` |
| **Frame Corruption** | Data is arriving, but failing the Checksum (FCS). | `ethtool -S <interface> | grep crc` |

### 2. Troubleshooting Workflow: The Logic of Elimination

#### Step A: ARP and arping
Standard `ping` uses IP addresses. If it fails, it could be a Layer 3 routing issue. To isolate the problem to Layer 2, use `arping`.

* **Logic**: `arping` sends a Who-has request directly via the Ethernet broadcast address. It does not use IP routing; it only cares if a MAC address responds to an IP on the same wire.
* **Command**: `arping -I <interface> <target_ip>`
* **Result**:
    - `Success`: You receive a reply with a MAC address. Layer 1 and Layer 2 are officially functional.
    - `Failure`: No reply. This means either the cable is bad (L1), the VLAN is wrong, or the target is not on this physical segment.

#### Step B: Verify Local Neighbor Resolution
Before a packet can be sent, the OS must know the destination's MAC address.
*   **Command:** `ip neigh show`
*   **Logic:** 
    *   `REACHABLE`: Layer 2 is working perfectly.
    *   `INCOMPLETE` or `FAILED`: The system sent an ARP Request ("Who has IP X.X.X.X?"), but no one answered. This indicates a cabling issue, a firewall on the target, or a VLAN mismatch.

#### Step C: Inspect Raw Frames
If the neighbor table is empty, observe the wire directly to see if any traffic is arriving.
*   **Command:** `tcpdump -e -i <interface>`
*   **Logic:** The `-e` flag forces `tcpdump` to show the **Ethernet Header**. If you see incoming frames from other MAC addresses but your system isn't responding, the issue is in your software's parsing logic.

#### Step D: Check for VLAN Tagging
In professional environments, a single wire may carry traffic for multiple isolated networks.

VLANs (Virtual Local Area Networks) function by inserting a 4-byte "tag" into the Ethernet frame. If the sender and receiver do not expect the same tag, the kernel discards the frame before it reaches the application.

*   **Logic:** If a TAP device is configured for VLAN 10, but the incoming traffic is untagged (or tagged for VLAN 20), the kernel will silently drop the frames at Layer 2.
* **Command** (See all tagged traffic): `tcpdump -i <interface> -e vlan`
    * Logic: The `-e` flag shows the MAC header, and vlan filters for frames containing an 802.1Q tag. If this returns nothing, but a standard `tcpdump` shows traffic, the incoming traffic is untagged.
* **Command** (Check interface configuration): `ip -d link show <interface>`
    * Logic: The `-d` (details) flag reveals if the interface is bound to a specific VLAN ID (VID). Look for a line saying `vlan protocol 802.1Q id <number>`.

#### Step E: DHCP
DHCP (Dynamic Host Configuration Protocol) is technically an Application Layer (L7) protocol because it uses UDP/IP to communicate. However, it is fundamentally tied to Layer 2 initialization.

```bash
# check fi dhclient/network manager is running
ps aux | grep -i dhc

# Try to get dhcp lease manually
dhclient -v eth0

# observer the DISCOVER, OFFER, REQUEST, ACK
```


### 3. Fixing Layer 2 Issues

*   **For ARP Failures:** 
    1. Ensure the target device is actually on the same physical segment/VLAN.
    2. Check if the target has "ARP Filtering" or a strict firewall enabled.
    3. Manual Fix: Force an entry (for testing only): `sudo arp -s <ip_address> <mac_address>`.
*   **For MAC Table Issues:**
    *   **Fix:** If using a Bridge or TAP, ensure the bridge is learning MACs: `bridge fdb show`.
*   **For MTU (Maximum Transmission Unit) Mismatches:**
    *   **Logic:** If a frame is larger than the allowed size (usually 1500 bytes), it will be dropped.
    *   **Fix:** `sudo ip link set dev <interface> mtu 1400` (reducing size often bypasses hidden encapsulation overhead).