## TCP state 

### Connection Establishment
The goal of this phase is to synchronize Sequence Numbers and confirm that both hosts are ready to communicate.
1. The Normal Three-Way Handshake
    This is the standard process where one side acts as a passive listener and the other as an active initiator.

    * `CLOSED`: The default state where no connection exists.
    * `LISTEN`(Server): The server waits for an incoming connection request on a specific port.
    * `SYN-SENT`(Client): The client sends a SYN (Synchronize) packet and waits for a response.
    * `SYN-RECEIVED`(Server):  The server receives the SYN, sends a `SYN-ACK`, and waits for the client’s final acknowledgement.
    * `ESTABLISHED`: The client sends an ACK. Both sides transition to this state, and data transfer begins.

2. Simultaneous Open
    This occurs if two hosts perform an "Active Open" to each other at the same time.

    | State Transition (Side A) | Action | State Transition (Side B) |
    | :--- | :--- | :--- |
    | **CLOSED** $\rightarrow$ **SYN-SENT** | Both send **SYN** | **CLOSED** $\rightarrow$ **SYN-SENT** |
    | **SYN-SENT** $\rightarrow$ **SYN-RECEIVED** | Both receive **SYN**, send **ACK** | **SYN-SENT** $\rightarrow$ **SYN-RECEIVED** |
    | **SYN-RECEIVED** $\rightarrow$ **ESTABLISHED** | Both receive **ACK** | **SYN-RECEIVED** $\rightarrow$ **ESTABLISHED** |

3. TCP Fast Open (TFO)
    TFO allows data to be included in the initial `SYN` packet if a cryptographic cookie was exchanged in a **previous session**. This reduces latency by allowing the server to process data before the handshake is fully complete.
    * Prerequisite: The client must have already connected to the server once to obtain a cryptographic cookie.
    * `SYN` + `Data`(Client): The client sends a SYN packet that includes the TFO Cookie and the first piece of Application Data (e.g., an HTTP GET request).
    * `SYN-SENT` $\rightarrow$ `ESTABLISHED`(Client): The client doesn't wait; it enters a state where it can continue sending data if the window allows, though it still waits for the server's acknowledgement.
    * `SYN-RECEIVED` + `Data Processing` (Server): The server verifies the cookie. If valid, it immediately passes the data to the application and sends a SYN-ACK acknowledging both the sequence number and the data.
    * `ESTABLISHED` (Server): The server can send a response (data) back immediately within its SYN-ACK or right after it, without waiting for the final ACK from the client.

    | State Transition (Client) | Action | State Transition (Server) |
    | :--- | :--- | :--- |
    | **CLOSED** $\rightarrow$ **SYN-SENT** | Sends **SYN** + **Cookie** + **Data** | **LISTEN** $\rightarrow$ **SYN-RECEIVED** |
    | **SYN-SENT** $\rightarrow$ **ESTABLISHED** | Receives **SYN-ACK** | Processes **Data**, sends **SYN-ACK** |
    | **ESTABLISHED** | Sends **ACK** | **SYN-RECEIVED** $\rightarrow$ **ESTABLISHED** |

### Data Transfer Phase
In the ESTABLISHED state, data flows bidirectionally. The state machine remains here until a "Close" command is issued or a "Reset" (`RST`) packet is received.
1. If a "Close" command is issued
    If the application decides to shut down the connection gracefully, it issues a Close command. This triggers the FIN (Finish) flag.
2. If a "Reset" (`RST`) packet is received
    If the connection is terminated by a RST packet, a FIN is NOT sent.
    * The Process: A RST is an immediate, unilateral termination. It usually happens due to an unrecoverable error, a timeout, or a security measure (like a firewall killing a session).
    * The Result: The state machine transitions directly to the CLOSED state. It doesn't wait for acknowledgments or finish sending buffered data. It’s an "abort" rather than a "close."

### Connection Termination
TCP is full-duplex, meaning each direction of the connection must be closed independently. This usually requires a four-packet exchange.
1. The Normal Close (Active vs. Passive)
    One side initiates the close (Active Closer), and the other responds (Passive Closer).
    * `FIN-WAIT-1` (Active): The side initiating the close sends a FIN packet. It is waiting for an ACK or a FIN from the peer.
    * `CLOSE-WAIT` (Passive): The receiver gets the FIN and sends an ACK. It informs the local application that the peer is done sending.
    * `FIN-WAIT-2` (Active): The initiator receives the ACK for its FIN. It now waits for the peer to send its own FIN.
    * `LAST-ACK` (Passive): Once the passive side’s application is ready, it sends its own FIN and waits for the final ACK.
    * `TIME-WAIT` (Active): The initiator receives the peer's FIN, sends the final ACK, and enters a timed wait period.
    * `CLOSED`: After the timer or the final ACK, the resources are released.

2. Simultaneous Close
    This occurs when both sides send a FIN simultaneously before receiving one from the peer.
    * ESTABLISHED $\rightarrow$ FIN-WAIT-1: Both send FIN.
    * FIN-WAIT-1 $\rightarrow$ CLOSING: Both receive the peer's FIN, But both machines waited for `ACK`, since it received `FIN` it send an ACK. They have not yet seen the ACK for their own FIN. if you see any one machine prespecitve it sent a `FIN` -> waiting for `ACK` but got `FIN`
        * In normal close it would go for `FIN-WAIT-2`,  but here since it already received `FIN` it directly go for `CLOSING`
    * `CLOSING` $\rightarrow$ `TIME-WAIT`: Action: Both sides receive the `ACK` for the `FIN` they originally sent in Step 1.
    * `TIME_WAIT` $\rightarrow$ `CLOSED` : Both sides wait for $2 \times MSL$ (Maximum Segment Lifetime) to ensure any wandering packets are cleared from the network before the port is reused.

3. Combined Close (FIN + ACK)
    The passive closer may send its ACK (for the peer's FIN) and its own FIN in a single packet.
    * The active closer moves directly from FIN-WAIT-1 to TIME-WAIT, skipping FIN-WAIT-2.

### The TIME-WAIT State and the RST Flag
**The Purpose of TIME-WAIT**
The TIME-WAIT state typically lasts for twice the Maximum Segment Lifetime (2MSL), which can range from 30 seconds to 4 minutes. It serves two functions:
1. Ensuring Reliability: It allows the final ACK to be retransmitted if it is lost, preventing the peer from being stuck in the LAST-ACK state.
2. Preventing Ghost Segments: It ensures that old, delayed packets from the current connection expire so they are not misinterpreted as part of a new connection using the same ports.

**The Reset (RST) Flag**
If an unrecoverable error occurs, a host sends a packet with the RST bit set. This forces the receiver to jump directly to the CLOSED state, bypassing the normal four-way handshake. This is considered an "Abnormal Termination."


### Things go south
There are a few "hidden" mechanisms and edge cases that govern how TCP behaves when things don't go according to plan

1. What if one side doesn't send a response or ACK?
    TCP is a "pessimistic" protocol—it assumes the network is unreliable. If a response doesn't arrive, it triggers Retransmission Logic.
    **The Retransmission Timer (RTO)**
    Every time TCP sends a segment (like a SYN or Data), it starts a timer.
    * The Wait: If an ACK isn't received before the timer expires, TCP assumes the packet was lost.
    * The Action: It retransmits the segment and doubles the timer value (Exponential Backoff).
    * The Limit: After a certain number of retries (usually 5 to 15, depending on the OS), TCP gives up, sends a RST to the local application, and transitions to CLOSED.

    **Half-Open Connections (The "Zombified" State)**
    What if the client crashes or the cable is pulled after the connection is established?
    * The Problem: The server thinks the connection is still ESTABLISHED and keeps memory allocated for it.
    * The Solution (Keep-Alives): TCP has an optional Keep-Alive timer (often 2 hours by default). If no data has moved, the server sends a tiny "Are you there?" probe. If the client doesn't ACK after several probes, the server kills the connection.

### Missed & important
1. The "Half-Close" State
    There is a specific functional state called TCP Half-Close.
    * An application can call shutdown(SHUT_WR). This sends a FIN to the peer, signaling: "I am done sending data, but I am still willing to receive data from you."
    * The initiator stays in FIN-WAIT-2 potentially forever if the other side stays in CLOSE-WAIT and keeps sending data.

2. SYN Flooding and SYN Cookies
    In the `SYN-RECEIVED` state, the server allocates resources (backlog queue).
    * The Risk: An attacker can send thousands of SYN packets but never send the final ACK, filling the server's memory.
    * The Corner Detail: To fight this, many systems use SYN Cookies. The server doesn't allocate memory immediately. Instead, it encodes the connection info into the Sequence Number of the SYN-ACK. It only "remembers" the connection once the client sends back a valid ACK.

3. Zero Window & Persist Time
    What if the receiver is overwhelmed and sends an ACK with a Window Size of 0?
    * The sender stops sending data and enters a "Persist" state.
    * The Trap: If the "Window Update" packet (telling the sender "I have space now!") gets lost, both sides would wait forever.
    * The Fix: The sender uses a Persist Timer to periodically send "Window Probes" to force the receiver to announce its current window size.