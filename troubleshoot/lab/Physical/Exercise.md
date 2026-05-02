Complete the setup mentioned in previous document

# Exercise: Layer 1 Troubleshooting in a Multi-Node Multicast Lab

In this exercise, two Alpine Linux VMs are connected to a "Virtual Hub" using **Multicast Sockets**. This simulates a physical Ethernet segment where both machines share the same wire.

### Follow this logic to trouble shoot
1. Check the Hardware Detection
2. Check the Driver Availability
3. Check the Administrative Status `ip link show`, should be `up`
5. Check the Flags in `ip link show`
4. Check the Compatibility and Performance `ethtool -S <interface>` check teh error stats

## Device & Driver

### Drive missing
When a Network Interface Card (NIC) is physically present but lacks a driver, the system cannot use it. You can confirm this state by checking three specific points:

#### Check 
1. Check `ip link`: The interface (like `eth0`) will be missing from the list.
2. Confirm Hardware Presence: Run `grep "" /sys/bus/pci/devices/*/class`. If you see `0x020000`, the Ethernet hardware is detected by the motherboard.
3. Check Driver Link: Check the directory `/sys/bus/pci/devices/[PCI_ADDRESS]/driver`. If the driver file does not exist, the hardware is not connected to any software.
> replace [PCI_ADDRESS] with actual address

#### solution
Load the driver

### Wrong driver
Even if a driver is attached to a PCI device, it may be the "wrong" one. This usually happens in two ways:
* Generic Match: The device appears in `ip link`, but performance is poor because the driver only supports basic functions.
* Incompatible Match: The device is bound to a driver, but `ip link` does not show the interface because the driver cannot translate the hardware signals into a network object.

#### Check

1. Check Current Binding: Use `ls -la /sys/bus/pci/devices/[PCI_ADDRESS]/driver`
2. Check Kernel Logs: Run `dmesg | grep -i "[PCI_ADDRESS]"`. Look for error messages such as "probe failed" or "invalid hardware," which indicate the driver is struggling to communicate with the device.

#### Solution
1. Detach the device and driver `echo "[PCI_ADDRESS]" > /sys/bus/pci/drivers/[WRONG_DRIVER_NAME]/unbind`
2. Attach the correct driver to the device `echo "[PCI_ADDRESS]" > /sys/bus/pci/drivers/[CORRECT_DRIVER_NAME]/bind`

```bash
localhost:~# echo "0000:00:03.0" > /sys/bus/pci/drivers/virtio-pci/unbind
localhost:~# ls -la /sys/class/net/*/device
ls: /sys/class/net/*/device: No such file or directory
localhost:~# echo "0000:00:03.0" > /sys/bus/pci/drivers/virtio-pci/bind 
localhost:~# ls -la /sys/class/net/*/device
lrwxrwxrwx    1 root     root             0 May  1 09:49 /sys/class/net/eth0/device -> ../../../virtio0
localhost:~#
```

## Hardware 

### The "Broken Cable" (Physical Disconnect)
**The Goal:** Simulate a cable being pulled out of the NIC on Node A.

1.  **Simulation:** On Node A, press `Ctrl + a`, then `c` to enter the QEMU Monitor. 
    1. Type: `set_link n1 off`
    2. Press `Ctrl + a`, then `c` to return to Alpine.
2.  **Detection:** 
    *   Command: `ip link show eth0`
    *   **Observed Logic:** The output shows `NO-CARRIER`. This indicates the virtual hardware no longer detects an electrical signal from the multicast "hub."
    * **Error**: *No explict error*

    ```bash
    (qemu) set_link n1 off
    (qemu) 
    ip link show
    1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
        link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    2: eth0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc pfifo_fast state DOWN qlen 1000
        link/ether 52:54:00:12:34:02 brd ff:ff:ff:ff:ff:ff
    localhost:~# ping -c 2 192.168.1.1
    PING 192.168.1.1 (192.168.1.1): 56 data bytes
    ^C
    --- 192.168.1.1 ping statistics ---
    2 packets transmitted, 0 packets received, 100% packet loss
    localhost:~# 
    ```
    Link is broken in the Node a only, other system still works fine, but when we ping the broken IP it cannot reach
3.  **The Fix:** 
    *   Enter QEMU Monitor: `set_link n1 on`.
    *   **Logic:** Restores the signal path. `ip link` should now show `LOWER_UP`.


#### Scenario B: The "Admin Error" (Interface Disabled)
**The Goal:** Simulate an interface that is physically connected but software-disabled.

1.  **Simulation:** Inside Node B, run:
    `ip link set eth0 down`
2.  **Detection:**
    *  Command: `ip link show eth0`
    * **Observed Logic:** The `UP` flag disappears from the brackets `<BROADCAST,MULTICAST>`. The state explicitly says `DOWN`.
    * **Error**: *Network unreachable*

    ```bash
    localhost:~# ip link set eth0 down
    localhost:~# ping -c 3 192.168.1.1
    PING 192.168.1.1 (192.168.1.1): 56 data bytes
    ping: sendto: Network unreachable
    localhost:~# ip link
    1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
        link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    2: eth0: <BROADCAST,MULTICAST> mtu 1500 qdisc pfifo_fast state DOWN qlen 1000
        link/ether 52:54:00:12:34:01 brd ff:ff:ff:ff:ff:ff
    localhost:~# 
    ```
3.  **The Fix:**
    *   Command: `ip link set eth0 up`
    *   **Logic:** Re-activates the driver to begin processing frames from the virtual wire.

### Scenario C: The "Missing Component" (Driver Failure)
**The Goal:** Simulate a kernel where the network driver has crashed or was never loaded.

1.  **Simulation:** Inside either Node, run: `modprobe -r virtio_net`. This remove driver 
2.  **Detection:**
    *   **Step 1:** `lspci | grep -i ethernet` (The hardware is still detected on the PCI bus).
    *   **Step 2:** `ip link show` (The interface `eth0` is missing entirely).
    *   **Observed Logic:** If `lspci` sees the card but `ip link` does not, the issue is purely a **Driver (Layer 1 software)** failure.
        * Check the `dmesg` for driver logs, 
        * `lsmod` will list all the installed drivers, search for the driver `virtio_net`
    * **Error**: *Network unreachable*
    ```bash
    localhost:~# modprobe -r virtio_net
    localhost:~# ip link show
    1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
        link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    localhost:~# 
    localhost:~# ping -c 3 192.168.1.1
    PING 192.168.1.1 (192.168.1.1): 56 data bytes
    ping: sendto: Network unreachable
    ```

3.  **The Fix:**
    *   Command: `modprobe virtio_net`
    *   **Logic:** Re-inserts the VirtIO networking module into the Linux kernel.

### Scenario D: "Hardware Errors" (Packet Loss/Corruption)
In a real-world scenario, a bad cable or a failing NIC port would cause CRC errors or dropped packets. 

**Check** 
Check Errors: `ethtool -S eth0` or `cat /sys/class/net/<interface>/statistics/rx_errors` some othet stats (`rx_missed_errors, rx_crc_errors, tx_aborted_errors`)