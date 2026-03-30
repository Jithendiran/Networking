## Session Multiplexing

In Transmission Control Protocol (TCP), a connection is uniquely identified by a 4-tuple: (Source IP, Source Port, Destination IP, Destination Port)

When the Operating System (OS) establishes this connection, it assigns a File Descriptor (FD). This integer acts as a handle for the application to read from or write to that specific socket. In traditional networking (HTTP/1.1), this FD represents a single, serial stream of data. If the application needs to send a second request, it must either wait for the first to finish or open a new FD (a new 4-tuple), which consumes significant system memory and CPU cycles for the SYN/ACK handshake.

### Session Multiplexing

Session Multiplexing is a mechanism at the Session Layer (Layer 5 of the OSI model) that allows multiple logical communication sequences to coexist within a single physical or transport-layer connection.

While the Transport Layer (Layer 4) manages the "pipe" (error correction, ordering, and flow control), the Session Layer manages the "conversations" inside that pipe. Even though the IP and Port (the 4-tuple) remain identical, the data packets are tagged with a Session ID or Stream Identifier.

### The Architecture of a Multiplexed Frame

To achieve this, the protocol must wrap raw data into a structured unit called a Frame. A typical multiplexed frame contains:
* Type: Defines if the frame is a header, data, or a control signal.
* Flags: Indicates if this is the start or end of a session.
* Stream/Session ID: A unique integer (e.g., Stream 1, Stream 3, Stream 5) that tells the receiver which logical conversation this data belongs to.
* Payload: The actual application data.

**The Receiver's Role**
Upon receiving a packet from a specific Port/IP, the receiver examines the Session ID. Instead of dumping all data into one buffer, it sorts the data into different "virtual" buffers based on the ID. This allows the application to process "Session A" and "Session B" independently, even though they arrived on the same wire.

### Why This Approach is Necessary
The shift from single-session connections to multiplexed sessions solves three critical bottlenecks:
1. Mitigation of Head-of-Line (HoL) Blocking
    In non-multiplexed connections, if a large file (e.g., a 100MB video) encounters a lost packet, the entire connection stalls. All subsequent requests are blocked. In a multiplexed environment, a delay in Session ID 1 does not stop the delivery of frames for Session ID 2.
2. Reduced Resource Overhead
    Every TCP connection requires a "Control Block" in the OS kernel. For a server handling 100,000 users, opening 10 connections per user (1,000,000 sockets) would exhaust the available file descriptors and memory. Multiplexing allows those same 100,000 users to be handled via 100,000 sockets, with the Session ID handling the internal complexity.
3. Persistent State Management
    - The Session Layer maintains the "state" of the interaction. If the underlying transport connection momentarily drops, the Session ID can be used to resume the conversation exactly where it left off once the connection is re-established, providing a seamless experience for the application.
    - The Session ID tells the server who is talking. Since the IP and Port only tell the server which computer is talking, the Session ID identifies the specific user or specific tab in a browser, ensuring User A doesn't accidentally get User B's bank statement.

**Real-World Implementation: HTTP/2 and HTTP/3**
In HTTP/2, multiplexing is the core feature. A browser opens one TCP connection to a server. When the user loads a page, the browser assigns a unique ID to every image, CSS file, and script. All these files are sent simultaneously. The server interlaces the frames, and the browser reassembles them using the IDs.

In HTTP/3, this is taken further using QUIC, where the multiplexing is built directly into the transport mechanism to further reduce latency and improve handovers between networks (like switching from Wi-Fi to 5G).