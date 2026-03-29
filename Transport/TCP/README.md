## TCP
TCP is a stateful protocol. Each endpoint (Client and Server) maintains a state to track the progress of a connection. Transitions between states occur based on three triggers:
1. User Commands: An application requests to open or close a connection.
2. Incoming Packets: Receiving a packet with specific flags (SYN, ACK, FIN, RST).
3. Timeouts: A specific timer (like the 2MSL timer) expires.

TCP (Transmission Control Protocol) is a connection-oriented protocol that enables full-duplex communication, allowing data to flow in both directions simultaneously between a sender and a receiver. It is designed for high reliability, ensuring that all transmitted data packets arrive at their destination intact and in the correct sequence.

[State](./state.md)

[Descriptor](./descriptor.md)