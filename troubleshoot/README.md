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
*   **Logic:** Look for "Speed" and "Duplex." If one side is "Half-Duplex" and the other is "Full-Duplex," collisions will occur, causing massive packet loss despite having a "Link."


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