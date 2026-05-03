## Datalink setup

```bash
qemu-system-x86_64 -enable-kvm -m 512 \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -netdev socket,id=n1,mcast=230.0.0.1:1234 \
  -device virtio-net-pci,netdev=n1,mac=52:54:00:12:34:01 \
  -netdev user,id=n2 \
  -device virtio-net-pci,netdev=n2,mac=52:54:00:12:34:03 \
  -serial mon:stdio \
  -serial telnet:localhost:4444,server,nowait \
  -nographic
```

### Understanding QEMU Serial Options
New options

```
-serial mon:stdio \
-serial telnet:localhost:4444,server,nowait \
```

QEMU uses serial options to create communication channels between the host computer and the virtual machine. Without these, a headless virtual machine has no way to display text or receive keyboard input.

#### 1. `-serial mon:stdio`
This option performs two tasks in the main terminal window:
*   **Serial Console:** It connects the virtual machine’s primary text output (ttyS0) to the terminal. This allows for logging in and running commands.
*   **Monitor (mon):** It embeds the QEMU Monitor into the same window. The monitor is a control layer used to manage the hardware of the virtual machine (QEMU terminal) while it runs.
*   **Combined Access:** It allows switching between the operating system and the hardware controller using a keyboard shortcut (Ctrl+a, then c).

#### 2. `-serial telnet:localhost:4444,server,nowait`
This option creates an additional, network-based communication port:
*   **Secondary Port:** it creates a second serial interface (ttyS1) inside the virtual machine.
*   **Network Listener (telnet):** It turns port 4444 on the host computer into a gateway. A second terminal can connect to this port to see different data.
*   **Server Mode:** The virtual machine acts as a host, waiting for a connection to be made.
*   **Nowait:** This prevents the virtual machine from pausing at startup. It ensures the boot process begins immediately even if no one has connected to the telnet port yet.

### Why these are required
*   **Multi-Tasking:** These options allow for two separate terminal windows to be open at once for the same virtual machine. One window can stay dedicated to monitoring network traffic (like ARP frames), while the other is used to type commands.
*   **Control:** The monitor access is necessary to pause, reset, or quit the virtual machine when it is running in a mode without a graphical interface.
*   **Accessibility:** In a headless environment, there is no virtual screen. These options provide the only method to interact with the system.

### How to enter telnet
> telnet localhost 4444.

### How to exit telnet
The Standard Escape Sequence
1. Press `Ctrl + ]` (hold the Control key and press the right square bracket).
  - This will bring you to the `telnet>` prompt.
2. Type quit and press Enter.

## Previous command
```bash
qemu-system-x86_64 -enable-kvm -m 512 \
-drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
-netdev socket,id=n1,mcast=230.0.0.1:1234 \
-device virtio-net-pci,netdev=n1,mac=52:54:00:12:34:01 \
-nographic
```

Here why `-serial mon:stdio` is not required?

The `-nographic` option is a shorthand command. It automatically configures several settings to make the virtual machine work in a text-only terminal.

### What `-nographic` does automatically
When `-nographic` is used, QEMU performs the following actions by default:
*   **Disables Video:** It shuts off the graphical VGA output.
*   **Redirects Serial:** It automatically redirects the first serial port (`serial0`) to the current terminal's input and output (stdio).
*   **Multiplexes the Monitor:** It automatically combines the QEMU Monitor and the serial console into that same terminal window.


### Why the manual `-serial` option was suggested earlier
While `-nographic` provides one terminal, it only provides **one**. 

The manual options were provided to achieve **two** specific goals that `-nographic` cannot do alone:

1.  **Multiple Windows:** The `-serial telnet...` option creates a *second* path. This allows two different terminal windows to be open at the same time. This is necessary if one window needs to show live network logs (`tcpdump`) while the other window is used to type commands.
2.  **Explicit Control:** By adding `-serial mon:stdio` manually alongside the telnet option, it ensures the first terminal remains the primary controller while the telnet port acts as a secondary experimental interface.

### Summary
The command in the prompt works without `-serial mon:stdio` because `-nographic` includes that functionality by default. The extra lines are only required when more than one communication channel is needed for the same virtual machine.


## Config

Node B config remain same as [Basic.md](../basic.md)

### Node A
```bash
qemu-system-x86_64 -enable-kvm -m 512 \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -netdev socket,id=n1,mcast=230.0.0.1:1234 \
  -device virtio-net-pci,netdev=n1,mac=52:54:00:12:34:01 \
  -netdev user,id=n2 \
  -device virtio-net-pci,netdev=n2,mac=52:54:00:12:34:03 \
  -serial mon:stdio -serial telnet:localhost:4444,server,nowait \
  -nographic
```

Inside `VM`
1. Enable internet interface: `ip link set eth1 up`
2. Get IP address from DHCP: `udhcpc -i eth1`

```bash
localhost:~# ip addr
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN qlen 1000
    link/ether 52:54:00:12:34:01 brd ff:ff:ff:ff:ff:ff
3: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP qlen 1000
    link/ether 52:54:00:12:34:03 brd ff:ff:ff:ff:ff:ff
    inet 10.0.2.15/24 scope global eth1
       valid_lft forever preferred_lft forever
    inet6 fec0::5054:ff:fe12:3403/64 scope site dynamic flags 100 
       valid_lft 86270sec preferred_lft 14270sec
    inet6 fe80::5054:ff:fe12:3403/64 scope link 
       valid_lft forever preferred_lft forever
``` 
`10.0.2.15` is a NAT ip, can access internet but it is not possible to access this IP from outside world without portforwarding

3. Ensure eth1 is fully routed : `setup-interfaces -r`
4. The '-1' flag automatically picks the first available mirror: `setup-apkrepos -1`
5. Reconfig apt list : `echo "https://dl-cdn.alpinelinux.org/alpine/v3.23/main" > /etc/apk/repositories && echo "https://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories` 
6. Update internal repo : `apk update`
7. Install tcpdump : `apk add tcpdump`


```bash
qemu-system-x86_64 -enable-kvm -m 512 \
  -drive file=alpine-virt-3.23.4-x86_64.iso,media=cdrom \
  -netdev socket,id=n1,mcast=230.0.0.1:1234 \
  -device virtio-net-pci,netdev=n1,mac=52:54:00:12:34:01 \
  -netdev user,id=n2 \ # for internet
  -device virtio-net-pci,netdev=n2,mac=52:54:00:12:34:03 \       # for internet
  -serial mon:stdio -serial telnet:localhost:4444,server,nowait \ # for accessing 2nd terminal
  -nographic
localhost:~# ip link set eth1 up
localhost:~# udhcpc -i eth1 # for internet access to download package

localhost:~# ip addr
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <BROADCAST,MULTICAST> mtu 1500 qdisc noop state DOWN qlen 1000
    link/ether 52:54:00:12:34:01 brd ff:ff:ff:ff:ff:ff
3: eth1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP qlen 1000
    link/ether 52:54:00:12:34:03 brd ff:ff:ff:ff:ff:ff
    inet 10.0.2.15/24 scope global eth1
       valid_lft forever preferred_lft forever
    inet6 fec0::5054:ff:fe12:3403/64 scope site dynamic flags 100 
       valid_lft 86270sec preferred_lft 14270sec
    inet6 fe80::5054:ff:fe12:3403/64 scope link 
       valid_lft forever preferred_lft forever
localhost:~# setup-interfaces -r
localhost:~# setup-apkrepos -1
localhost:~# cat /etc/apk/repositories 
/media/sr0/apks
localhost:~# echo "https://dl-cdn.alpinelinux.org/alpine/v3.23/main" > /etc/apk/repositories
localhost:~# echo "https://dl-cdn.alpinelinux.org/alpine/v3.23/community" >> /etc/apk/repositories
localhost:~# cat /etc/apk/repositories
https://dl-cdn.alpinelinux.org/alpine/v3.23/main 
https://dl-cdn.alpinelinux.org/alpine/v3.23/community
localhost:~# 
localhost:~# apk update
localhost:~# apk add tcpdump
```

8. Disable ARP in `Node B` this has to done before executing `ping` on either of the node, execute this on destination node B `ip link set dev eth0 arp off`

```bash
localhost:~# ip link set dev eth0 arp off
localhost:~# ip link
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <BROADCAST,MULTICAST,NOARP,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast stat0
    link/ether 52:54:00:12:34:02 brd ff:ff:ff:ff:ff:ff
```
