The TCP segment (or header) is the structural unit that carries control information and data across the network. Understanding each field is a prerequisite for understanding how the state machine transitions from one phase to another.


## I. The TCP Segment Structure (Descriptor)
* A TCP segment is the unit of data that TCP creates and transmits across a network. When an application passes a stream of bytes to TCP, TCP does not send that stream as-is. It breaks the stream into chunks, attaches a header to each chunk, and sends each chunk as one segment.
* The word "segment" is TCP-specific terminology. The same unit is called a "datagram" at UDP, a "packet" at IP, and a "frame" at Ethernet. 
* A TCP segment travels inside an IP packet as its payload (Encapsulation) means TCP is inside IP packet.
* A TCP segment has two parts:
```
[ TCP Header ] [ Data Payload ]
```
* The header carries all the control information. The payload carries the actual application data. 
* The header is mandatory; the payload is optional
*  a segment can carry zero bytes of data (used for ACKs, SYN, FIN, RST).


##  TCP Header

A standard TCP header is **20 bytes** long (without options). It is organized into 32-bit (4-byte) rows. Header size 5 * 4 bytes = 20 bytes

If options are present, the header extends up to 60 bytes. Every field has a fixed position and a fixed bit width.

```
            16bits                          16bits
--------------------------------|--------------------------------
0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Source Port          |       Destination Port        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Data |           |U|A|P|R|S|F|                               |
| Offset| Reserved  |R|C|S|S|Y|I|            Window             |
|       |           |G|K|H|T|N|N|                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           Checksum            |         Urgent Pointer        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options (Variable Length)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             Data                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

In networking, data is often visualized in 32-bit words.


| Field Name | Size (Bits) | Description |
| :--- | :--- | :--- |
| **Source Port** | 16 | The port number of the sending application. |
| **Destination Port** | 16 | The port number of the receiving application. |
| **Sequence Number** | 32 | Tracks the byte order. In a `SYN` packet, this is the Initial Sequence Number (ISN). |
| **Acknowledgment Number** | 32 | If the `ACK` flag is set, this contains the next byte expected from the peer. |
| **Data Offset** | 4 | Specifies the size of the TCP header (where the data begins). |
| **Reserved** | 3 | Always set to zero; reserved for future use. |
| **Control Flags** | 9 | Control bits (NS, CWR, ECE, URG, ACK, PSH, RST, SYN, FIN). |
| **Window Size** | 16 | Used for flow control; indicates how many bytes the receiver can accept. |
| **Checksum** | 16 | Used for error-checking the header and data. |
| **Urgent Pointer** | 16 | Indicates the end of "urgent" data if the `URG` flag is set. |
| **Options** | Variable | Optional settings like Maximum Segment Size (MSS) or Timestamps. |


### 1. Source Port — 16 bits
The port number of the sender. Port numbers range from 0 to 65535.Combined with the sender's IP address (from the IP header), this identifies the sending endpoint.
* Ports 0–1023: Well-known ports (HTTP=80, HTTPS=443, SSH=22). Reserved for system services.
* Ports 1024–49151: Registered ports. Applications can register these with IANA.
* Ports 49152–65535: Ephemeral ports. The OS assigns these dynamically to clients making outgoing connections.
A TCP connection is uniquely identified by the 4-tuple: (Source IP, Source Port, Destination IP, Destination Port). Two connections with the same destination but different source ports are completely separate connections.

### 2. Destination Port — 16 bits
The port number of the intended receiver. The receiving machine uses this to hand off the segment to the correct application process.
When a web server receives a segment with Destination Port = 80, the kernel delivers it to the process listening on port 80. This demultiplexing — one machine, many connections — is entirely based on the destination port combined with the full 4-tuple.

### 3. [Sequence Number — 32 bits](./Sequence_Number.md)

### 4. Acknowledgment Number — 32 bits
This field is only meaningful when the ACK flag is set (which it is in virtually every segment after the initial SYN).

The Acknowledgment Number contains the sequence number of the next byte the sender of this segment expects to receive. It is an implicit confirmation that all bytes up to (Acknowledgment Number − 1) have been received correctly.

Cumulative acknowledgment: TCP does not acknowledge each byte individually. One ACK acknowledges all bytes received up to that point. If segments arrive out of order, TCP holds the out-of-order data and does not advance the Acknowledgment Number until the gap is filled.

### 5. Data Offset — 4 bits
Also called the Header Length field. It specifies where the TCP header ends and the data payload begins.

The value is measured in 32-bit words (4 bytes each). Minimum value is 5 (5 * 4 = 20 bytes, the minimum header). Maximum value is 15 (15 * 4 = 60 bytes, the maximum header with options).

The receiving side reads this field first to know how many bytes belong to the header and how many belong to the payload.

### 6. Reserved — 3 bits (originally 6 bits, trimmed as flags were added)
These bits are set to zero and ignored. They exist to preserve 32-bit alignment and to allow future protocol extensions without changing the fixed structure.

### 7. Control Flags — 9 bits (historically 6, expanded over time)
Each flag is one bit. A flag value of 1 means it is set (active). These flags govern the meaning and purpose of the segment. A single segment can have multiple flags set simultaneously.

* **URG — Urgent**
    Normally, TCP is a "stream" protocol—data is processed in the exact order it arrives (In-band). "Out-of-band" was meant to let a piece of data "jump the line" to be handled immediately by the application, even if other data was still sitting in the buffer.

    The classic example is Telnet or FTP.
    Imagine you are running a long command on a remote server and realize it’s a mistake. You hit Ctrl+C to stop it. Without an "Urgent" signal, that "Stop" command would get stuck at the back of the line behind all the data the server is already busy sending you. The URG flag was meant to tell the server: "Stop what you're doing and look at this specific command right now."

* **ACK — Acknowledgment**
    When set, the Acknowledgment Number field is valid. Every segment after the first SYN carries this flag set. The ACK flag is the most commonly set flag in any TCP session — essentially all segments in an established connection have ACK = 1.

* **PSH — Push**
    When set, the receiver should pass the data to the application immediately instead of waiting to accumulate more data in the buffer. The sender uses this to signal "the application wants this data delivered now."

    In practice, modern TCP stacks and applications do not rely heavily on PSH for correctness. Most implementations set PSH on the last segment of a write call. The receiving TCP may or may not honor it strictly.

* **RST — Reset**
    When set, the connection must be immediately terminated. There is no graceful handshake. The receiver drops all buffered data and considers the connection dead.

    RST is sent in these situations:
    - A segment arrives for a port with no listening socket.
    - A connection is in an inconsistent state.
    - ...
    A segment with RST set requires no acknowledgment. The connection simply ends.

* **SYN — Synchronize**
    Used exclusively during the three-way handshake to establish a connection. When SYN is set, the Sequence Number field carries the ISN of the sender, not a reference to payload bytes.

    * The client's first segment: SYN = 1, ACK = 0
    * The server's response: SYN = 1, ACK = 1 (this is the only SYN+ACK segment)
    * The client's final handshake segment: SYN = 0, ACK = 1

    After the handshake, SYN is never set again in that connection. A SYN seen mid-connection is treated as an error.

* **FIN — Finish**
    Used during connection termination. When a side sets FIN = 1, it signals that it has no more data to send. Like SYN, FIN consumes one sequence number and must be acknowledged.

    FIN does not immediately close the connection — it closes only one direction of the data flow. The other side can still send data. This is called a half-closed connection. The connection fully closes only when both sides have sent and acknowledged FIN.

### 8. Window Size — 16 bits
This field is the receiver's flow control advertisement. It tells the sender how many bytes the receiver is currently willing to accept — i.e., how much free space exists in the receiver's buffer.

**Unit: bytes**
Range: 0 to 65,535 bytes (raw field). With the Window Scale option (see Options section), the effective maximum scales up to about 1 gigabyte.

How it works: The sender must not have more unacknowledged bytes in flight than the Window Size value. If the window is 8192, the sender can send 8192 bytes and then must wait for ACKs to advance the window before sending more.

Window = 0: A zero-window advertisement means the receiver's buffer is full. The sender stops transmitting and periodically sends a window probe (a 1-byte segment) to check when the receiver reopens the window.

This field changes with every segment — the receiver updates it based on how fast the application is consuming data from the buffer.

### 9. Checksum — 16 bits
A mandatory error-detection field. The checksum is computed over three parts:
1. A pseudo-header (not actually transmitted) containing: Source IP, Destination IP, protocol number (6 for TCP), and TCP segment length.
2. The TCP header itself.
3. The TCP payload.
The inclusion of IP addresses in the checksum computation (via the pseudo-header) makes the checksum sensitive to misdelivered packets — if IP routes a packet to the wrong machine, TCP's checksum will fail because the destination IP in the pseudo-header will not match.

The checksum uses one's complement addition. It detects single-bit errors reliably. It does not correct errors — a failed checksum causes the segment to be silently discarded. TCP's retransmission mechanism handles recovery.

Note: On modern hardware, checksum computation is often offloaded to the NIC (network interface card), not the CPU.

### 10. Urgent Pointer — 16 bits
Only meaningful when the URG flag is set. Contains an offset from the Sequence Number field. The byte at (Sequence Number + Urgent Pointer) is the last byte of urgent data.

Urgent data was designed to allow out-of-band signaling (like sending an interrupt/abort signal to a remote process). The practical behavior is inconsistently defined and implemented. Modern protocol design avoids the Urgent mechanism entirely.

## TCP Options — Variable Length (0 to 40 bytes)
Options extend the TCP header beyond 20 bytes. Each option follows a Type-Length-Value (TLV) structure, except for the two single-byte options. Options must be padded with zeros to align the total header to a 32-bit boundary.

1. Option 0 — End of Option List (1 byte)
    A single zero byte that marks the end of the options section. Used only when padding is needed after the last real option.

2. Option 1 — No Operation / NOP (1 byte)
    A single byte with value 1. Used as padding between options to align subsequent options to word boundaries. Has no functional effect.
3. Option 2 — Maximum Segment Size (MSS) — 4 bytes
    ```
    Kind=2 | Length=4 | MSS Value (2 bytes)
    ```
    Exchanged only during the SYN and SYN-ACK segments. Each side advertises the largest payload it is willing to receive in a single segment.

    The MSS does not include the TCP or IP headers — it is the payload size only. A typical MSS on Ethernet is 1460 bytes (Ethernet MTU 1500 − 20 bytes IP header − 20 bytes TCP header).

    Each side sets its own MSS independently. The sender uses the receiver's advertised MSS as the upper bound for segment size. If no MSS option is received, TCP defaults to 536 bytes.

    MSS prevents IP fragmentation by ensuring segments fit within the path MTU. MSS is not negotiated — each side simply announces its own limit, and both sides respect the other's value.

4.  Option 3 — Window Scale — 3 bytes

    ```
    Kind=3 | Length=3 | Shift Count (1 byte)
    ```
    The Window Size field is 16 bits, capping the window at 65,535 bytes. On high-bandwidth, high-latency links (e.g., satellite, intercontinental fiber), this cap severely limits throughput.

    The Window Scale option introduces a shift count (0–14). The actual window size becomes:
    
    $$\text{Effective Window} = \text{Window Size field} \times 2^\text{(Shift Count)}$$
    Maximum effective window: $65535 \times 2^{14} = 1073725440 \text{ bytes }(~1 GB)$.

    This option is exchanged only in SYN and SYN-ACK. Both sides must offer it for scaling to be active. If one side does not include Window Scale in its SYN, scaling is disabled for the entire connection.

5. Option 4 — Selective Acknowledgment Permitted (SACK-Permitted) — 2 bytes
    ```
    Kind=4 | Length=2
    ```
    Exchanged only in SYN and SYN-ACK. Signals that the sender of this option supports SACK (Selective Acknowledgment). Both sides must signal support for SACK to be used.
6. Option 5 — Selective Acknowledgment (SACK) — Variable (10 to 34 bytes)

    ```
    Kind=5 | Length | Block 1 Left Edge | Block 1 Right Edge | ...
    ```

    Standard TCP acknowledgment is cumulative — it can only acknowledge a contiguous block of received bytes. If segments arrive out of order (e.g., segment 2 is lost but segments 3, 4, 5 arrive), the receiver can only ACK up to the last contiguous byte before the gap.

    SACK allows the receiver to inform the sender exactly which non-contiguous blocks have been received. Each SACK block is a pair of 32-bit sequence numbers (Left Edge, Right Edge) representing a received range.

    Up to 4 SACK blocks fit in one segment (each block = 8 bytes; 4 blocks = 32 bytes + 2 bytes overhead = 34 bytes max).

    Effect: The sender retransmits only the missing segments rather than everything after the gap. This dramatically improves performance on lossy or reordered networks.

7. Option 8 — Timestamps — 10 bytes
    ```
    Kind=8 | Length=10 | Timestamp Value (4 bytes) | Timestamp Echo Reply (4 bytes)
    ```
    Serves two purposes:

    * Purpose 1 — Round-Trip Time Measurement: The sender places the current time in Timestamp Value. The receiver echoes it back in Timestamp Echo Reply. The sender computes RTT as (current time − echoed timestamp). This gives a more accurate RTT measurement than inferring it from ACK timing alone, especially when retransmissions complicate measurement.
    * Purpose 2 — PAWS (Protection Against Wrapped Sequence Numbers): On very high-speed links, sequence numbers can wrap around quickly. An old duplicate segment arriving late could have a sequence number that appears valid in the current window. Timestamps allow TCP to reject such old segments: if the timestamp on an incoming segment is older than the most recent timestamp seen, the segment is from an old era and is discarded.

## Data Payload
The payload carries the actual application data. Its size is:
```
Payload Size = Total Segment Size − (Data Offset x 4)
```
Payload size is bounded above by:
* The MSS advertised by the receiver
* The sender's congestion window (cwnd)
* The receiver's flow control window
A segment with zero-length payload is valid and common. ACK-only responses, SYN, FIN, and RST segments often carry no payload.


## Todo
* TCP option 6,7
* remaining falgs
(already in claude https://claude.ai/chat/7520b620-1cfe-46ca-950c-b79dd5236263)