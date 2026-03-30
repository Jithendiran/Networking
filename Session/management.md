## Session management
Once a session is established, the session layer governs dialog control: the rules about which side may send data at any given moment. There are three modes.

* **Simplex**: Only one party ever sends. The other only receives. This is uncommon in modern interactive protocols but appears in broadcast and telemetry systems.
* **Half-duplex**: Both parties can send, but not at the same time. Only one side holds the token (the right to transmit) at any moment. When that side is done, it explicitly passes the token to the other side. Half-duplex dialog requires the session layer to track token ownership and enforce the constraint that only the token holder may transmit. This was common in early terminal protocols like IBM's LU 6.2 and in X.225 (the OSI session protocol).
* **Full-duplex**: Both parties may send simultaneously. The session layer does not restrict transmission direction. This is the mode used by modern TCP-based sessions — the underlying transport handles collision avoidance, and there is no token concept.

The choice of dialog mode is negotiated during session establishment and does not change mid-session.


### Synchronization and checkpointing — the recovery mechanism
It exists to solve a specific problem: what happens when a long-running data transfer is interrupted mid-way?
- Without synchronization, the answer is: restart from the beginning. 
- With synchronization, the answer is: restart from the last checkpoint.

The session layer accomplishes this by inserting synchronization points into the data stream. A synchronization point is a numbered marker that both sides agree to recognize. When a synchronization point is acknowledged by both sides, it means both sides have confirmed that all data up to that point has been received correctly. If the session fails after that point, the transfer can resume from the last acknowledged synchronization point rather than from zero.

**There are two types of synchronization points:**
* **Major synchronization points**: These divide the data stream into major segments called activities. Once a major synchronization point is acknowledged, neither side may go back before it. The data before the major sync point is committed — it will not be retransmitted. Moving past a major sync point requires explicit acknowledgment from both sides.
* **Minor synchronization points**: These divide the data within a major segment into smaller checkpoints. Minor sync points do not require acknowledgment before the session proceeds — they are inserted speculatively. If a failure occurs, the session can restart from the last acknowledged minor sync point within the current major segment. If no minor sync point within the segment was acknowledged, the session restarts from the beginning of the current major segment (the last major sync point).

### Activity management — organizing sessions into discrete units

The session layer introduces the concept of an activity. An activity is a discrete, self-contained logical unit of work within a session. Activities are bounded by major synchronization points. The session layer can suspend and resume activities, which means a long session can pause one unit of work, begin another, and later return to the suspended one.

Activity management matters for protocols that need to interleave different kinds of work within a single session. A file transfer protocol, for example, might send one file as one activity, suspend it mid-way if a higher-priority message arrives, handle that message as a second activity, and then resume the first activity.

The key properties of activities are:

* One activity runs at a time within a session. Activities do not overlap. When one activity is suspended, the session layer records the suspension point (a major sync point is written) and the new activity begins from a clean state. When the suspended activity resumes, it restarts from its last confirmed state.
* Activities are identified by an activity identifier assigned when the activity begins. This identifier persists across suspensions and resumptions, allowing both sides to unambiguously reference the same unit of work.


### Session resumption — avoiding full re-establishment

Full session establishment is expensive. It involves multiple round trips, cryptographic operations, and sometimes user authentication (typing a password or approving a multi-factor prompt). For sessions that are interrupted and need to restart, re-doing the full establishment adds latency that users notice.

Session resumption is the mechanism by which a session can restart from a known point without full re-establishment. The session layer (or its equivalent in a real-world protocol) enables resumption by preserving state across the interruption.

In TLS, session resumption works through two mechanisms:
* **Session tickets**: The server encrypts the session state (master secret, cipher suite, etc.) and sends it to the client as an opaque ticket. The client stores the ticket and presents it in the next connection. The server decrypts the ticket, recovers the session state, and resumes without a full handshake. The advantage is that the server stores no per-session state — it is entirely in the ticket.
* **TLS 1.3 with 0-RTT**. In TLS 1.3, a client that has a valid session ticket can send application data in the very first message of the new connection, before the handshake is complete. This eliminates one full round trip. The trade-off is that 0-RTT data does not have forward secrecy and is vulnerable to replay attacks, so it is only appropriate for idempotent requests.


In the OSI model, resumption is handled through the combination of synchronization points and activity identifiers. After a reconnection, both sides exchange the identifier of the suspended activity and the serial number of the last acknowledged synchronization point, then continue from there