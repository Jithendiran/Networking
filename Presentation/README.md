## The Presentation Layer 

The session layer establishes and manages a named conversation between two processes. Once that conversation is open, data starts moving. But there is a problem that the session layer does not solve: the two sides may not store, represent, or interpret data the same way.

Consider two machines exchanging a number. One machine stores integers in big-endian byte order (most significant byte first). The other stores them in little-endian byte order (least significant byte first). If the first machine sends the number 1000 as raw bytes, the second machine reads the same bytes and interprets them as a completely different number. Neither machine did anything wrong. The mismatch is in how data is represented, not in whether it was delivered.

This is the problem the presentation layer exists to solve. Its job is to ensure that data produced by the sending application arrives at the receiving application in a form the receiving application can understand — regardless of differences in hardware architecture, operating system, programming language, or internal data formats.

The presentation layer sits at Layer 6 of the OSI model. Below it is the session layer (Layer 5), which manages the conversation. Above it is the application layer (Layer 7), which produces and consumes the actual data. The presentation layer is the translator between them.

Three responsibilities define this layer completely: translation (converting between different data representations), encryption and decryption (protecting data in transit), and compression and decompression (reducing the size of data before it is sent).

### The three core responsibilities of the presentation layer

Every mechanism in this layer connects to one of these three responsibilities, and they must be understood in this order because they build on each other.

1. **[Translation](./Translation.md)** is the first responsibility. Data leaving an application exists in that application's internal format. Before it crosses a network, it must be converted into a common, agreed-upon representation that the receiver can decode regardless of its own internal format. This is called syntax conversion or data translation.

2. **[Encryption and decryption](./Security.md)** is the second responsibility. Data in transit crosses networks that neither side controls. The presentation layer is responsible for transforming data into a form that cannot be read by anyone who intercepts it, and for reversing that transformation at the receiving end.

3. **[Compression and decompression](./Compression.md)** is the third responsibility. Before data is sent, it can often be made smaller. Smaller data crosses the network faster and consumes less bandwidth. The presentation layer applies compression algorithms on the sending side and decompression on the receiving side.

In practice, these three operations happen in a defined order.

- On the sending side: the application data is first translated into a common format, then compressed, then encrypted. 
- On the receiving side, the order reverses exactly: decrypt first, then decompress, then translate into the application's local format.

**How it implements**

**TLS** implements encryption and decryption. TLS is implemented as a library (OpenSSL, BoringSSL, LibreSSL, rustls) that sits between the TCP socket and the application. From an OSI perspective, TLS spans the boundary between the presentation and session layers — it establishes sessions (with session tickets and resumption), authenticates peers (with certificates), and encrypts data (with symmetric ciphers). This is why the OSI model's clean boundary between Layer 5 and Layer 6 does not appear cleanly in TLS.

**HTTP** content negotiation (via Content-Type, Accept, Content-Encoding, Accept-Encoding headers) implements both translation and compression negotiation. The HTTP layer is technically application layer (Layer 7) in OSI, but it carries out presentation layer responsibilities.

**Serialization libraries** (protobuf libraries, Jackson for JSON, JAXB for XML) implement translation — converting between in-memory objects and wire formats. These are application-level components in the TCP/IP stack but their function is presentation-layer.

**Codec libraries** (libx264, libvpx, FFmpeg) implement media compression and decompression.

**Operating system facilities** — the htonl/ntohl family of functions in POSIX, and the equivalent in Windows Sockets — implement byte order conversion.


The general principle is: the presentation layer is concerned with how data is represented — its encoding, its compression, its encryption. The application layer is concerned with what data means and what to do with it — the semantics of requests and responses, the business logic, the state machine of the application protocol.

In HTTP, Content-Type and Content-Encoding are **presentation-layer** concerns. The method (GET, POST), the URL, the response status code, and the meaning of the body are **application-layer** concerns.

In TLS, the handshake and cipher negotiation are presentation-layer concerns. The application data that flows through the TLS tunnel is handed off to the application layer unchanged.

## Negotiation
Every layer has its own negotiation, and they are independent of each other.

Here is the full picture, layer by layer.

* Layer 1 — Physical. No negotiation in the traditional sense. Hardware is pre-configured: cable type, signal voltage, connector standard. Auto-negotiation on Ethernet (IEEE 802.3u) is the one exception — two NICs automatically agree on speed (10/100/1000 Mbps) and duplex mode when a cable is plugged in. This is handled entirely in hardware.

* Layer 2 — Data Link. Ethernet frames carry a fixed structure defined by the standard. No handshake needed. Switches learn which MAC addresses are on which ports by observing traffic. Wi-Fi (802.11) has an association handshake where a device and access point agree on supported rates and security mode.

* Layer 3 — Network. IP itself does no negotiation — packets just carry a version field and header. ICMP handles error signaling. DHCP is where a host negotiates its IP address, subnet mask, gateway, and DNS server from a server on the local network. BGP and OSPF (routing protocols) negotiate routes between routers using their own handshakes.

* Layer 4 — Transport. TCP does the most visible negotiation here: the three-way handshake (SYN, SYN-ACK, ACK) where both sides agree on initial sequence numbers, window sizes, and TCP options like maximum segment size (MSS) and selective acknowledgment (SACK) support. UDP does zero negotiation — it sends and forgets.

* Layer 5 — Session. As covered in the session layer reference: session parameters (dialog mode, checkpointing, timeout), authentication, and session identifier assignment. This is explicit bilateral negotiation before any application data flows.

* Layer 6 — Presentation. As covered in the presentation layer reference: TLS negotiates cipher suites, key exchange algorithms, certificate verification, and compression support during the TLS handshake. HTTP content negotiation (Accept, Accept-Encoding, Content-Type) handles format and compression agreement at this level.

* Layer 7 — Application. Each protocol has its own handshake. HTTP/1.1 negotiates keep-alive. HTTP/2 negotiates over the Upgrade header or ALPN (Application-Layer Protocol Negotiation, which runs inside TLS). SMTP has a greeting and capability exchange (EHLO). FTP negotiates transfer mode and data channel. DNS has no connection-level negotiation because it typically runs over UDP.
