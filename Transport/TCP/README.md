I'm new to networking
I had started with TCP state management, descriptors, sync flow
i need you to write in a detailed way, even if i miss something you have to include that in the correct place
this will be a be a go to place for me, after 6 months if i come back, just by reading i should able to understand the concepts without any hiccups, my learning approach is brick by brick and independent to dependent thing

Explain this with simple english, no analogym professional english, only use third person view (should not use words like i, you,...)

i would like to start session layer

```

```

## TCP
TCP is a stateful protocol. Each endpoint (Client and Server) maintains a state to track the progress of a connection. Transitions between states occur based on three triggers:
1. User Commands: An application requests to open or close a connection.
2. Incoming Packets: Receiving a packet with specific flags (SYN, ACK, FIN, RST).
3. Timeouts: A specific timer (like the 2MSL timer) expires.

TCP (Transmission Control Protocol) is a connection-oriented protocol that enables full-duplex communication, allowing data to flow in both directions simultaneously between a sender and a receiver. It is designed for high reliability, ensuring that all transmitted data packets arrive at their destination intact and in the correct sequence.

[State](./state.md)

[Descriptor](./descriptor.md)