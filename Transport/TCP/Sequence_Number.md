# Sequence Number — 32 bits

## What It Is

TCP is a stream-oriented protocol. When an application hands data to TCP, TCP does not see "messages" or "blocks" — it sees a continuous stream of bytes. Every single byte in that stream gets a position number. That position number is the **sequence number**.

The Sequence Number field in a TCP segment header carries the position of the **first byte of the payload in that segment**. The receiver uses sequence numbers to:

- Reassemble segments that arrive out of order into the correct original stream
- Know which bytes have already been received
- Know which bytes are missing and need to be retransmitted


## Initial Sequence Number (ISN)

The sequence number does not start from 0. During the three-way handshake, each side independently picks a random starting number called the **Initial Sequence Number (ISN)**.

**Why random?**

Consider a situation where a connection closes and a new connection opens between the same two machines on the same ports (same 4-tuple). If sequence numbers always started from 0, a delayed segment from the old (dead) connection could arrive late and have a sequence number that falls inside the new connection's valid range. TCP would accept it as legitimate data — corrupting the stream silently.

A random ISN makes this practically impossible. The probability that an old segment's sequence number lands inside the new connection's window is negligible when both connections start from different random points.

**SYN consumes one sequence number**

During the handshake, the SYN segment carries no application data. But SYN still consumes one sequence number. This is because SYN itself must be acknowledged by the other side — and to be acknowledged, it must occupy a position in the sequence number space.

- Client sends SYN with Sequence Number = ISN (example: 9988)
- Server must ACK this SYN $\rightarrow$ sends Acknowledgment Number = 9989 (ISN + 1)
- The first real data byte from the client will carry Sequence Number = 9989

This is why the first data segment starts at ISN + 1, not ISN.


## Range

| Property | Value |
|---|---|
| Field size | 32 bits |
| Minimum value | 0 |
| Maximum value | 4,294,967,295 ($2^{32}$ − 1) |

After 4,294,967,295, the counter wraps back to 0. TCP arithmetic is designed to handle this wrap-around correctly using modular arithmetic — more on this below.


## How Application Data Is Split Into Segments

### The Byte Stream

When an application gives TCP 4,550 bytes to send, TCP assigns a sequence number to every byte in that chunk — not just the first byte. The numbering is continuous and starts from where the previous transmission left off.

If the ISN was 9988, and the SYN consumed sequence number 9988, and the ACK for the SYN consumed nothing (ACK segments carry no data and consume no sequence numbers — only SYN and FIN do), then:

- First data byte $\rightarrow$ Sequence Number 9989
- Second data byte $\rightarrow$ Sequence Number 9990
- ...
- 4,550th data byte $\rightarrow$ Sequence Number 9989 + 4549 = **14,538**

The formula for any byte's sequence number:

$$\text{Sequence Number of byte } n = \text{ISN} + 1 + (n - 1)$$

Or from the header perspective: the segment's header carries the sequence number of its **first byte**, and the remaining bytes follow consecutively from there.


### Segmentation

Physical networks have a limit on how large a single chunk of data can be. This limit is called the **Maximum Segment Size (MSS)**. On a typical Ethernet network, MSS is **1,460 bytes** (derived from the Ethernet MTU of 1,500 bytes minus 20 bytes for the IP header minus 20 bytes for the TCP header).

TCP cannot send all 4,550 bytes in one segment because 4,550 > 1,460. TCP slices the byte stream into segments, each carrying at most 1,460 bytes. The Sequence Number in each segment header is the position of that segment's first byte.


### Segmentation Table

Starting Sequence Number (ISN) = 9988. SYN consumed 9988. First data byte = 9989.

| Segment | Bytes Carried | First Byte Position | Last Byte Position | **Sequence Number in Header** |
|---|---|---|---|---|
| Segment 1 | 1,460 bytes | 1st to 1,460th | 9989 + 1459 = 11,448 | **9,989** |
| Segment 2 | 1,460 bytes | 1,461st to 2,920th | 11,449 + 1459 = 12,908 | **11,449** |
| Segment 3 | 1,460 bytes | 2,921st to 4,380th | 12,909 + 1459 = 14,368 | **12,909** |
| Segment 4 | 170 bytes | 4,381st to 4,550th | 14,369 + 169 = 14,538 | **14,369** |

Verification: 1,460 + 1,460 + 1,460 + 170 = **4,550 bytes** 


### How the Receiver Uses These Numbers

When all four segments arrive at the receiver (possibly out of order):

- Segment with Sequence Number 9,989 $\rightarrow$ goes first
- Segment with Sequence Number 11,449 $\rightarrow$ goes second
- Segment with Sequence Number 12,909 $\rightarrow$ goes third
- Segment with Sequence Number 14,369 $\rightarrow$ goes last

The receiver does not need to receive them in this order. Even if Segment 3 arrives before Segment 1, the receiver holds Segment 3 in a buffer and waits. Once Segment 1 arrives, the receiver fills positions 9,989 to 11,448 first, then continues in order. The sequence numbers are the positions — reassembly is just placing each segment at its correct position.


## What Happens When the Sequence Number Reaches $2^{32}$ − 1

### The Wrap-Around

The sequence number field is 32 bits. After 4,294,967,295, the next value is 0. This is called **sequence number wrap-around**.

TCP uses **modular arithmetic** (specifically, modulo $2^{32}$) for all sequence number comparisons. This means TCP does not compare sequence numbers as plain integers. It compares them as positions on a circular number line.

On this circular number line:

- 4,294,967,295 is immediately before 0
- A sequence number of 100 is "greater than" 4,294,967,200 because on the circle, 100 is ahead

The rule TCP uses: sequence number A is considered **greater than** B if the distance from B to A going forward (in the direction of increasing numbers, wrapping at $2^{32}$) is less than $2^{31}$.

The receiver handles this correctly as long as:

- No segment stays in the network longer than **MSL (Maximum Segment Lifetime)** — defined as 2 minutes (120 seconds) in RFC 793
- The sequence numbers do not wrap around more than once within that window

This brings up the actual problem with wrap-around, described next.


## The Wrapped Sequence Number Problem — PAWS

### When Does Wrap-Around Become a Problem?

Wrap-around is safe on normal networks because 4 GB of data takes a long time to transmit. On a 1 Mbps link, sending 4 GB takes:

$$\frac{4{,}294{,}967{,}295 \text{ bytes}}{125{,}000 \text{ bytes/sec}} \approx 34{,}359 \text{ seconds} \approx 9.5 \text{ hours}$$

No delayed duplicate segment from 9.5 hours ago will still be alive in the network.

But on a high-speed link, this changes drastically:

| Link Speed | Time to Exhaust $2^{32}$ Sequence Numbers |
|---|---|
| 1 Mbps | ~9.5 hours |
| 100 Mbps | ~5.7 minutes |
| 1 Gbps | ~34 seconds |
| 10 Gbps | ~3.4 seconds |

At 1 Gbps, the sequence number space wraps around in 34 seconds. A segment delayed by the network for, say, 30 seconds could arrive with a sequence number that has already been reused. The receiver cannot tell whether it is an old duplicate or a current valid segment. **This is the wrap-around problem.**


### The Consequence

If the receiver mistakenly accepts an old duplicate segment:

- It places wrong data at that position in the byte stream
- The data delivered to the application is silently corrupted — no error is reported
- TCP's checksum only detects bit-level corruption, not logical duplication of old data


### The Solution — PAWS (Protection Against Wrapped Sequence Numbers)

PAWS uses the **TCP Timestamps option** to solve this. Every segment carries a timestamp value set by the sender. The receiver remembers the most recent valid timestamp it has seen from the sender.

When a new segment arrives:

- If its timestamp is **greater than or equal to** the most recent stored timestamp $\rightarrow$ the segment is current $\rightarrow$ accept it
- If its timestamp is **less than** the most recent stored timestamp $\rightarrow$ the segment is old, from before the wrap-around $\rightarrow$ **discard it silently**

This works because timestamps always increase monotonically (they never go backward). Even if sequence numbers have wrapped around and restarted from a low number, the timestamp on the new segment will be higher than the timestamp on the old delayed duplicate. The receiver can distinguish them.

PAWS is defined in **RFC 7323** and is active on any connection where both sides have negotiated the Timestamps option during the handshake.


## The Same Sequence Number Problem — Ambiguity

### What "Same Sequence Number" Means

Two segments have the same sequence number when:

1. The sequence number counter wraps around and the new segment's first byte lands at the same number as an older segment that is still somewhere in the network (delayed duplicate)
2. Two separate connections between the same pair of endpoints (same 4-tuple) reuse similar sequence numbers

### Case 1 — Delayed Duplicate After Wrap-Around

This is exactly the problem described in the PAWS section above. The same sequence number appears twice in the receiver's valid window — once for the current segment and once for the old duplicate. Without PAWS (timestamps), TCP cannot distinguish them.

### Case 2 — Two Connections, Same 4-Tuple

When a connection closes and a new connection opens between the same two machines on the same source port and destination port, the new connection's ISN is chosen randomly. But there is still a small probability that the new ISN is close to the old connection's sequence number range, especially if the old connection had old segments still traveling through the network.

The random ISN selection reduces this probability significantly. Additionally, TCP enforces a **Quiet Time** — after a crash or restart, TCP waits for one MSL (2 minutes) before accepting new connections, to allow all segments from old connections to expire.

### Why This Is Dangerous

If a receiver accepts an old duplicate segment as valid current data:

- The byte stream is corrupted without any signal to the application
- The application reads wrong data and has no way to know unless it has its own application-level integrity check (like TLS does)

TCP on its own relies on random ISN + PAWS + MSL expiry to prevent this. These three together make the probability of sequence number collision negligible in practice.


# Sequence Number Wrap-Around — Concrete Example

## Setup

- Application data: **4,550 bytes**
- ISN (SYN consumed this): **4,294,967,199**
- First data byte starts at: **4,294,967,200**
- MSS: **1,460 bytes**


## Segmentation

| Segment | Bytes Carried | Sequence Number (Header) | Last Byte Sequence Number |
|---|---|---|---|
| Segment 1 | 1,460 bytes | **4,294,967,200** | 4,294,967,200 + 1459 = **4,294,968,659**  exceeds max |

Stop here. 4,294,967,200 + 1,459 = **4,294,968,659** but the maximum is **4,294,967,295**. Segment 1 itself crosses the boundary. TCP must handle the wrap mid-segment.


## How TCP Handles the Wrap Mid-Segment

The sequence number field wraps at $2^{32}$ using modulo arithmetic. The numbers do not stop — they continue from 0.

**Segment 1 carries bytes at these sequence numbers:**

```
4,294,967,200  -> byte 1
4,294,967,201  -> byte 2
4,294,967,202  -> byte 3
...
4,294,967,295  -> byte 96       <-> last number before wrap
0              -> byte 97       <-> wrap happens here
1              -> byte 98
2              -> byte 99
...
1,363          -> byte 1,460    <-> last byte of Segment 1
```

**Bytes before wrap:** 4,294,967,295 − 4,294,967,200 + 1 = **96 bytes**

**Bytes after wrap:** 1,460 − 96 = **1,364 bytes** (sequence numbers 0 through 1,363)

Segment 1 header carries Sequence Number = **4,294,967,200** (the first byte). The receiver knows the rest follow consecutively using modular arithmetic.


## All Four Segments After Wrap-Around

| Segment | Seq Number (Header) | Last Byte Seq Number | Bytes Carried |
|---|---|---|---|
| Segment 1 | 4,294,967,200 | 1,363 | 1,460 bytes (wraps mid-segment) |
| Segment 2 | 1,364 | 2,823 | 1,460 bytes |
| Segment 3 | 2,824 | 4,283 | 1,460 bytes |
| Segment 4 | 4,284 | 4,453 | 170 bytes |

Verification: 1,460 + 1,460 + 1,460 + 170 = **4,550 bytes** 

Next sequence number after this transfer = **4,454**


## Where the Danger Lives

Segment 1 carried bytes at sequence numbers **0, 1, 2 ... 1,363** after the wrap.

Now consider this scenario:

```
Segment 1 is sent.
It gets delayed inside the network — sitting in some router queue.

Meanwhile, the connection finishes. A new connection opens
between the same two machines on the same ports (same 4-tuple).

The new connection's ISN happens to start near 0
(random, but possible).

The new connection is now actively sending data
with sequence numbers in the range 0, 1, 2 ...

The old delayed Segment 1 finally arrives.
It carries sequence numbers 0 through 1,363.
The receiver's current window also covers 0 through 1,363.
```

The receiver **cannot tell the difference**. Both the old delayed segment and the current new segment carry the exact same sequence numbers. The receiver accepts the old duplicate — placing **wrong data** from the previous connection into the current connection's byte stream. The application reads corrupted data with no warning.



## How PAWS Stops This

Every segment carries a **timestamp value** set by the sender at the time of sending.

```
Old Segment 1 (delayed, from previous connection):
  Sequence Number : 4,294,967,200 (wraps to cover 0–1363)
  Timestamp       : 1,000          <- set when sent, long ago

New Segment (current connection, same seq number range):
  Sequence Number : 50
  Timestamp       : 9,870,000      <- set now, much higher
```

The receiver stores the **highest timestamp seen so far** from the sender.

When the old delayed Segment 1 finally arrives:

```
Stored highest timestamp : 9,870,000
Incoming segment timestamp : 1,000

1,000 < 9,870,000  ->  this segment is OLD  <-  discard silently
```

The corrupted data never enters the byte stream. The current connection continues cleanly.

