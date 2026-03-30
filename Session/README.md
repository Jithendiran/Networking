## What the session layer is and why it exists

The transport layer is responsible for moving bytes reliably between two endpoints. It guarantees delivery, handles retransmission, and manages flow control. But the transport layer has no concept of a conversation. It does not know whether the bytes crossing the wire are part of a login sequence, a file transfer, or a database query. It does not know when a conversation begins, how long it lasts, or what happens if the network drops mid-way.

The session layer fills this gap. Its job is to establish, manage, and terminate sessions — structured, named periods of communication between two application processes. A session is not the same as a TCP connection. A TCP connection is a pipe; a session is the agreement about how that pipe gets used over time.

The session layer sits at Layer 5 of the OSI model. Below it is the transport layer (Layer 4). Above it is the presentation layer (Layer 6). In the real-world TCP/IP stack, the session layer does not exist as a distinct layer — its responsibilities are absorbed partly into the transport layer, partly into application protocols, and partly into TLS. 


## The three core responsibilities of the session layer

The session layer is responsible for exactly three things. Every feature and protocol mechanism in this layer connects to one of these three responsibilities.

1. **[Establishment](./establishment.md)** — creating a session, agreeing on parameters, and authenticating identities before any application data flows.
2. **[Management](./management.md)** — keeping the session alive, tracking where in a long exchange the two sides are, and inserting recovery points (called synchronization points) so that if something goes wrong, the transfer does not have to restart from zero.
3. **[Termination](./termination.md)** — closing the session cleanly, flushing any buffered data, and releasing resources on both sides.

These three phases have a strict order. Management cannot happen before establishment. Termination cannot happen before management. This sequencing is not optional — it is what gives the session its structure.


## The relationship between sessions and TCP connections
This is a critical distinction to understand precisely. A TCP connection and a session are not the same thing.

A TCP connection is a transport-layer construct. It is defined by four values: source IP, source port, destination IP, destination port. It is established with the three-way handshake (SYN, SYN-ACK, ACK) and torn down with a four-way close (FIN, ACK, FIN, ACK). TCP knows nothing about sessions, authentication, or application state.

A session is an application-layer (or session-layer) construct. It represents an authenticated, named conversation between two application processes. It may span multiple TCP connections (if the first connection drops and is replaced), or it may coexist with many other sessions over the same TCP connection (as in HTTP/2 multiplexing).

**The four combinations are:**
* One session, one TCP connection: The classic case. A client opens one TCP connection, establishes one session, does its work, closes the session, and closes the TCP connection.

* One session, multiple TCP connections: A long-lived session where the underlying transport drops and reconnects. Because the session has an identifier and synchronization points, the application can resume after the new TCP connection is established, without restarting from the beginning. This is what makes resumable downloads possible. (like connection cutoff in between)

* Multiple sessions, one TCP connection: A keep-alive TCP connection that is reused for multiple sequential sessions (HTTP persistent connections are an example). Each session is independent; they share the transport but not the session identifier or state.

* Multiple sessions, multiple TCP connections: The general case in multiplexed protocols. HTTP/2 and HTTP/3 allow many independent streams (logical sessions) over a single transport connection.

## How modern systems implement session-layer functions
The OSI session layer as a distinct protocol implementation is rarely seen in modern networks. The functions it defines, however, are present in every real-world networked application — they are simply implemented by different layers or components.

* TLS (Transport Layer Security) handles session establishment, authentication (via certificates), and the session resumption mechanism (TLS session tickets and TLS 1.3 0-RTT). TLS sits between the transport layer and the application layer, which places it at the boundary of the presentation layer in OSI terms. However, its session establishment and resumption functions directly correspond to OSI session-layer responsibilities.

* HTTP sessions are implemented at the application layer using cookies. The browser sends a session cookie with every request; the server maps the cookie value to stored session state. HTTP itself is stateless — the session layer behavior is added by the application framework on top of HTTP.

* RPC frameworks (gRPC, Apache Thrift, XML-RPC) implement session-like constructs with connection pooling, credential propagation, and request/response correlation identifiers.

* WebSockets establish a persistent, full-duplex session over an upgraded HTTP connection. The WebSocket upgrade handshake is a session-establishment event; the session persists until either side sends a close frame.

* SSH (Secure Shell) has one of the most explicit session-layer implementations in common use. An SSH connection is a transport-layer connection. Within that connection, SSH multiplexes multiple channels, each of which is a session. Each channel has its own state machine, flow control, and lifecycle independent of the others. SSH uses a transport-layer connection (TCP). SSH itself provides Secure Transport Service, but because it requires a lower-level protocol to handle the actual routing and packet delivery (IP, TCP), it is generally viewed as sitting "above" the transport layer.

## Session layer security considerations

The session layer is a target for several classes of attacks. Understanding these is part of understanding the session layer itself.

**Session hijacking.** An attacker who obtains a valid session ID can impersonate the legitimate client. The attacker does not need the client's credentials — only the session token. Defenses include using unpredictable session IDs (high entropy), binding sessions to the client's IP address or TLS certificate, and using the `HttpOnly` and `Secure` cookie flags.

**Session fixation.** An attacker tricks the server into accepting a session ID chosen by the attacker (rather than generated by the server). If the victim then authenticates using that session, the attacker already knows the ID and can access the session. The defense is to regenerate the session ID at the point of authentication, so the pre-authentication ID is never usable after login.

**Session replay.** An attacker captures a valid session message and retransmits it later to achieve the same effect. Defenses include nonces (one-time values included in session messages), timestamps, and sequence numbers that allow the receiver to detect and reject replayed messages.

**Session timeout.** Sessions that never expire are permanently exploitable if the session token is ever leaked. The session layer should enforce idle timeouts (close the session if no activity is observed within a window) and absolute timeouts (close the session after a maximum duration regardless of activity). Choosing appropriate timeout values requires balancing security against user experience.

**Cross-site session attacks (in HTTP contexts).** When session identifiers are stored in cookies, additional web-specific attacks apply: cross-site request forgery (CSRF), where a malicious page causes the victim's browser to send a request with the victim's session cookie, and cross-site scripting (XSS), where injected JavaScript reads the session cookie. CSRF is mitigated by same-site cookie policies and CSRF tokens; XSS is mitigated by `HttpOnly` cookies and content security policies.
