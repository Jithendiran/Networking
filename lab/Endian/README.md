## Endianness (Byte Order)
Endianness defines the order in which individual bytes of a multi-byte data type (such as an integer) are stored in computer memory. A single multi-byte number is composed of sequential memory addresses; Endianness dictates which byte occupies the lowest address.

## The Two Modes of Storage
To understand Endianness, consider the 32-bit hexadecimal integer: `0x12345678`. This value requires four bytes of memory.
* `MSB` (Most Significant Byte) - The left most `0x12`.
* `LSB` (Least Significant Byte) - The right most `0x78`.

1. **Big-Endian**
    The Most Significant Byte (MSB) is stored at the lowest memory address. This matches the human convention of reading numbers from left to right.
    - Memory layout: `12 | 34 | 56 | 78`

2. **Little-Endian**
    The Least Significant Byte (LSB) is stored at the lowest memory address. This is the standard for most modern processors (including x86 and x64 architectures).
    - Memory layout: `78 | 56 | 34 | 12`

### The Networking Logic: Why Byte Order Matters

Computers often run on different hardware architectures. If a Little-Endian machine transmits the integer `0x12345678` by sending the bytes in its internal memory order (78, 56, 34, 12) to a Big-Endian receiver, the receiver will interpret the data as `0x78563412`. The value is fundamentally corrupted.

To ensure consistent communication, the Internet Protocol (IP) suite mandates a universal standard: Network Byte Order, which is defined as Big-Endian.
* Host Byte Order: The internal storage order of the local machine (usually Little-Endian).
* Network Byte Order: The standardized order for data transmission (always Big-Endian).

Every multi-byte field in a network header (such as an Ethernet Type, IP Length, or TCP Port) must be converted to Network Byte Order before transmission and converted back to Host Byte Order upon reception.

### Implementation

The standard C library provides macros in the `<arpa/inet.h>` header to handle these conversions. These macros detect the local architecture's Endianness at compile time and perform the necessary byte-swapping only if required.

#### POSIX standard for network byte order conversion

These are sufficient for traditional IPv4 networking, as IPv4 addresses are 32 bits and port numbers are 16 bits.

| Function | Name | Purpose |
| :--- | :--- | :--- |
| `htons()` | **H**ost **to** **N**etwork **S**hort | Converts a 16-bit integer (e.g., port numbers) from Host to Network order. |
| `htonl()` | **H**ost **to** **N**etwork **L**ong | Converts a 32-bit integer (e.g., IP addresses) from Host to Network order. |
| `ntohs()` | **N**etwork **to** **H**ost **S**hort | Converts a 16-bit integer from Network to Host order. |
| `ntohl()` | **N**etwork **to** **H**ost **L**ong | Converts a 32-bit integer from Network to Host order. |

#### 64-Bit Extensions

For protocols requiring 64-bit integers, the standard POSIX functions are inadequate. Systems complying with modern standards (such as those providing `endian.h` in Linux) offer functions to handle 8-byte integers.

| Function | Description |
| :--- | :--- |
| `htobe64()` | **H**ost **to** **B**ig-**E**ndian 64-bit |
| `htole64()` | **H**ost **to** **L**ittle-**E**ndian 64-bit |
| `be64toh()` | **B**ig-**E**ndian to **H**ost 64-bit |
| `le64toh()` | **L**ittle-**E**ndian to **H**ost 64-bit |


#### Explicit Endianness Conversion
Beyond the standard Network (Big-Endian) to Host conversion, engineers often encounter scenarios where they must explicitly convert data to a specific endianness regardless of the host architecture (e.g., when reading binary files or parsing specific hardware protocols). The following macros allow for explicit transformations:

* **Host-to-Big-Endian:** `htobe16()`, `htobe32()`, `htobe64()`
* **Host-to-Little-Endian:** `htole16()`, `htole32()`, `htole64()`
* **Big-Endian-to-Host:** `be16toh()`, `be32toh()`, `be64toh()`
* **Little-Endian-to-Host:** `le16toh()`, `le32toh()`, `le64toh()`


#### Implementation Note
These extended functions are typically found in `<endian.h>` (on Linux/glibc) or `<sys/endian.h>` (on BSD/macOS). When writing portable, low-level networking code, verify the availability of these headers on the target operating system. If they are unavailable, manual bit-shifting or byte-swapping macros are employed as a fallback.

#### Operational Rule
1.  **Transmission:** Before writing any multi-byte integer into a raw buffer (e.g., a packet struct), call `htons()` or `htonl()`.
2.  **Reception:** After reading a multi-byte integer from a raw buffer, call `ntohs()` or `ntohl()` to restore the data to the machine’s local format.
3.  **Single Bytes:** Single-byte values (8-bit characters or `uint8_t`) do not require conversion, as they lack an internal sequence to reorder.

## Program
1. [Manual](./byte_swapper.c)
2. [Endian](./endian.c)
