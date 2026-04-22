## From Classful to Classless — The Complete Transition

### The Limitations of Classful Addressing
Classful addressing was designed in 1981 when the internet had fewer than 1,000 connected organisations. The design made a fundamental assumption that proved wrong: that three fixed sizes would be sufficient for all organisations that would ever exist.

By the late 1980s, this assumption had failed in multiple ways simultaneously.

#### Limitation 1 — The Two-Level Waste Problem
Classful addressing offered only three block sizes for assignment:
* Class A: 16,777,214 hosts
* Class B: 65,534 hosts
* Class C: 254 hosts
No organisation received exactly what it needed. Every assignment was either too small or too large.

**The Class B catastrophe**

If a company needed 300 addresses, a Class C (254) was too small. The company was forced to take a Class B (65,534). This resulted in 65,234 wasted addresses that could never be used by anyone else. Because the "split point" was fixed, there was no way to give a company exactly what it needed.

**The Class C fragmentation problem**

If an organisation needed 500 addresses, it received two Class C blocks — 192.168.1.0/24 and 192.168.2.0/24. These two blocks had no mathematical relationship in the routing table. Two separate entries were required in every router on the internet. This multiplied routing table size without adding usable address space.

#### Limitation 2 — Routing Table Explosion
In Classful addressing, a router sees every Class C network as a separate, unchangeable block.

If an ISP owns four contiguous  networks, All four are connected to the exact same router interface.
- Network 1: 192.168.0.0/24
- Network 2: 192.168.1.0/24
- Network 3: 192.168.2.0/24
- Network 4: 192.168.3.0/24

Because the Classful system does not allow these networks to be grouped, every router on the internet must save four separate lines in its RAM to reach this one ISP.

If this ISP has 1,000 customers, every router in the world must save 1,000 separate lines. As the internet grew to millions of networks, routers physically ran out of RAM (memory) to hold this massive list. There was no mathematical way to combine them because the "Class C" rule forced them to stay separate.

### How Engineers Planned the Next Move
In 1992 and 1993, the IETF (Internet Engineering Task Force) formed working groups to address the crisis. The problem was formally documented in RFC 1338 (1992) titled "Supernetting: an Address Assignment and Aggregation Strategy."

The analysis produced three conclusions:

* Conclusion 1 — The fixed class boundary is the root cause.
Removing the fixed split points between network and host portions would allow exact-fit allocations, eliminating waste.

* Conclusion 2 — Aggregation must be built into the addressing system.
ISPs must be able to hold one large block and advertise it as a single routing entry (No class), regardless of how it is subdivided internally among customers.

* Conclusion 3 — The split point must be explicitly carried with every address. Since the split point would no longer be encoded in the first bits of the address, it must be stated explicitly alongside the address. Every routing table entry and every address assignment must state its own prefix length.

These three conclusions became the design specification for CIDR.

### How to read classless

The number after the slash tells us exactly how many bits (out of 32) are reserved for the Network ID. The remaining bits are for Hosts (individual devices).

#### Step 1: Identify the "Split Point"

* Example: `192.168.1.32/27`
* Total Bits: 32
* Network Bits: 27
* Host Bits: 5 ($32 - 27 = 5$)

#### Step 2: Calculate the Block Size

1. Definition of Block Size

  Block Size is the total number of IP addresses contained within a specific sub-network (subnet). It represents the mathematical "step" or "increment" between one network address and the next. This value includes the network address, all usable host addresses, and the broadcast address.

2. Manual calculation

  1. Step A: Determine Bit Allocation
      * An IPv4 address has 32 bits total.
      * The prefix /27 dictates that the first 27 bits are locked for the Network.
      * Remaining bits for the Host: $32 - 27 = 5$ bits.
  
  2. Step B: Analyze the Host Octet (The 4th Octet)
      The first three octets (192.168.1) use 24 bits. The remaining 3 network bits and 5 host bits reside in the fourth octet.
        - Binary of `.32`: `0 0 1 | 0 0 0 0 0` (The | represents the /27 boundary).
        - Host Bits: The 5 zeros at the end.
  
  3. Step C: Calculate Possible Combinations
      * Since there are 5 bits reserved for the host, calculate how many different patterns can be made using only those 5 bits:
        1. 00000 (Decimal 0)
        2. 00001 (Decimal 1)
        3. 00010 (Decimal 2) ...and so on, until:
        4. 11111 (Decimal 31)
      * By counting every possible variation of those 5 bits, the result is exactly 32. This means the network "block" spans 32 units before the 27th bit (the network part) must change to start a new group.

3. Short hand calculation
    $$ Block\ Size = 2^{Host\ Bits} $$

    Using the `/27` example from above:

    * Host bits = 5
    * $2^5 = 32$
    * Block Size = 32 addresses.

    - The Block Size must account for every possible mathematical state the host bits can enter (in our case for host bit of `5` it can have unique 32bit format).
    - Since computer hardware processes data in powers of two, the size of a network must always be a power of two ($4, 8, 16, 32, 64, 128, 256$).
      * Computers do not use the decimal system (Base-10). They operate on Binary (Base-2), where every digit is a "bit" that can only be a 0 or a 1. Because there are only two choices per bit, every time a bit is added to the host portion, the number of possible addresses exactly doubles.

      * To understand why the block size is $2^n$, observe how the total number of unique patterns (addresses) grows as host bits are added:
        - 1 Host Bit: Can be 0 or 1: Total patterns: 2 ($2^1$)
        - 2 Host Bits: Can be 00, 01, 10, or 11: Total patterns: 4 ($2^2$)
        - 3 Host Bits: Can be 000, 001, 010, 011, 100, 101, 110, or 111: Total patterns: 8 ($2^3$)

        Conclusion: The "Block Size" is simply the count of every possible combination those bits can form. If a network has 5 host bits (like in the /27 example), the math dictates there are exactly $2 \times 2 \times 2 \times 2 \times 2 = 32$ unique patterns.


#### Step 3: Find the Network Boundaries
A network must start on a multiple of its own block size (if block size id `32`, then networks can only start at `0`, `32`, `64`, `96`, `128`, `160`, `192`, or `224`). 
  - This requirement exists because of how the Subnet Mask works. The mask acts like a "lock" that aligns with binary boundaries.
  - Standardization: If networks could start anywhere (like .35 or .42), routers would have to perform incredibly complex math to find them.
  - Binary Alignment: Because Block Sizes are powers of two ($8, 16, 32, \text{etc.}$), their starting points in binary always result in all host bits being 0.
  - Efficiency: By forcing networks to start on multiples, the router only needs to look at the "Network Part" to know exactly where a network begins and ends.

To read where the network starts and ends, we count in increments of that block size starting from `0`

Network Start: Previous block end + 1

Network End: start+(block size -1)

* Network 1: Starts at `.0`, Range: `.0` through `.31`
* Network 2: Starts at `.32` ($0 + 32$), Range: `.32` through `.63`
* Network 3: Starts at `.64` ($32 + 32$), Range: `.64` through `.95`
* Network 4: Starts at `.96` ($64 + 32$), Range: `.96` through `.127`

$$\cdots$$

Now let's find the boundries

The usable count is always the block size minus two.
$$ Usable\ Hosts = (2^h) - 2 $$

**Example: Reading 192.168.1.45/27**
1. Block Size: 32 (Calculated in Step 2).
2. Possible Network Starts: .0, .32, .64, .96...
3. Locate the Address: The number .45 falls between .32 and .64.

**The "Reading" Results:**
* Network Address: 192.168.1.32 (The start of the block).
* First Usable Host: 192.168.1.33 (Network + 1).
* Last Usable Host: 192.168.1.62 (One before the broadcast).
* Broadcast Address: 192.168.1.63 (The very last address in the block—one before the next network starts at .64).

### What Changed from Classful to Classless

#### Change 1 — The prefix length became explicit

**Classful**: The split point was determined by reading the first 1–4 bits of the address. No additional information was needed or possible. A router receiving the address 192.168.1.50 automatically knew it was Class C with a /24 split.

**Classless**: The split point is stated explicitly as a prefix length attached to the address. The address alone no longer implies a split. 192.168.1.50 with no prefix length attached is incomplete information.

```
Classful:   192.168.1.50        <- /24 implied by first octet 192
Classless:  192.168.1.50/24     <- /24 must be stated explicitly
            192.168.1.50/25     <- same address, different network boundary
            192.168.1.50/26     <- same address, different network boundary
```
The same address can belong to different-sized networks depending on the prefix length. The prefix length is now part of the address information.

#### Change 2 — The subnet mask became a required field everywhere
The subnet mask had existed before CIDR but was optional and only used internally within an organisation's network. Under classful rules, the mask was implied by the class.

Under CIDR, the subnet mask (or equivalent prefix length) must accompany every address in every context — routing table entries, interface configurations, address assignments, and routing protocol advertisements.

```
Routing table entry (classful era):   192.168.1.0   <- mask implied as /24
Routing table entry (CIDR era):       192.168.1.0/24  <- mask always explicit
```

#### Change 3 — Block sizes became arbitrary powers of 2
Under classful rules, only three host counts were possible per network: Class A or Class B or Class C.

Under CIDR, any prefix from /1 to /32 is valid:

| Prefix | Network bits | Host bits | Usable hosts |
|---|---|---|---|
| /8 | 8 | 24 | 16,777,214 |
| /16 | 16 | 16 | 65,534 |
| /24 | 24 | 8 | 254 |
| /25 | 25 | 7 | 126 |
| /26 | 26 | 6 | 62 |
| /27 | 27 | 5 | 30 |
| /28 | 28 | 4 | 14 |
| /29 | 29 | 3 | 6 |
| /30 | 30 | 2 | 2 |
| /32 | 32 | 0 | 1 (single host route) |

A /30 block with 2 usable addresses solved the point-to-point link waste problem completely.

#### Change 4 — Supernetting became possible

**Supernetting** is the combining of multiple smaller blocks into one larger routing entry. This is the aggregation mechanism that solved the routing table explosion.

Under classful rules, two Class C networks could not be combined because the class boundary was fixed at /24. Two separate routing entries were mandatory.

Under CIDR, if an ISP owns a contiguous block of addresses, it can advertise one summary entry:

```
ISP owns:
  200.10.0.0/24    (customer 1)
  200.10.1.0/24    (customer 2)
  200.10.2.0/24    (customer 3)
  200.10.3.0/24    (customer 4)

Internet sees only:
  200.10.0.0/22    <-- one entry covering all four /24 blocks
```

This is valid because `200.10.0.0/22` mathematically contains all addresses from `200.10.0.0` to `200.10.3.255`. Four routing entries became one.

### What Did Not Change in the Transition

#### Private address ranges — unchanged
CIDR did not alter private address they remain same:

| Block | Prefix | Usable range |
|---|---|---|
| 10.0.0.0/8 | /8 | 10.0.0.0 – 10.255.255.255 |
| 172.16.0.0/12 | /12 | 172.16.0.0 – 172.31.255.255 |
| 192.168.0.0/16 | /16 | 192.168.0.0 – 192.168.255.255 |

Under CIDR, organisations subnet these private ranges using any prefix length — not restricted to /8, /12, or /16. The boundaries above are the outer limits of each range, not the required subnet size.

### Loopback range — unchanged

`127.0.0.0/8` remains reserved for loopback. `127.0.0.1` is the standard loopback address. The entire /8 block is reserved though only `127.0.0.1` is commonly used.

#### Network and broadcast address rule — unchanged

Within any subnet, regardless of prefix length, the first address (all host bits = 0) is the network address and the last address (all host bits = 1) is the broadcast address. Both remain unusable for device assignment.

In the classless (CIDR) system, a network block can start and end at many different points within an octet. The "all zeros" and "all ones" rule applies to the host bits in binary, not necessarily the decimal number we see.

```
Network:   192.168.1.0/26     ->  network address:   192.168.1.0
                              ->  broadcast address: 192.168.1.63
                              ->  usable hosts:      192.168.1.1 – 192.168.1.62  (62 hosts)

Network:   192.168.1.32/27     ->  network address:   192.168.1.32
                              ->  broadcast address: 192.168.1.63
                              ->  usable hosts:      192.168.1.33 – 192.168.1.62  (62 hosts)
```

#### 0.0.0.0 default route — unchanged

`0.0.0.0/0` remains the default route. Under CIDR notation it is written explicitly as `/0` — zero network bits, meaning it matches all addresses but is the least specific possible match. Longest prefix matching means any more specific route always wins over the default.

`0.x.x.x` is also reserved. All addresses 0.0.0.1 through 0.255.255.255 are reserved with no assigned purpose. They cannot be used. No device may be configured with any address in this range. No routing entry points into this block on the public internet.

#### 255.255.255.255 limited broadcast — unchanged

`255.255.255.255` Remains the limited broadcast address. Sent to all devices on the local network. Not forwarded by routers. 

`255.x.x.x` — the rest of the block

All other addresses in this range — 255.0.0.0 through 255.255.255.254 — are reserved with no assigned use. No device may be configured with any address in this range.



#### 169.254.0.0/16 link-local — unchanged

Remains reserved for automatic link-local addressing (APIPA). Assigned when no DHCP server is reachable.
