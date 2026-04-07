## DCE RPC — Distributed Computing Environment Remote Procedure Call

###  What is a procedure call?
Before understanding remote procedure calls, the concept of a local procedure call must be understood.

In any program, code is organized into procedures (also called functions or subroutines). A procedure is a named block of code that performs a specific task. When one part of a program needs that task done, it calls the procedure by name, passes in any required inputs (called arguments), and waits for the result (called the return value) to come back.

This entire exchange — call, execute, return — happens inside one machine, in one process, in one memory space. Everything is local. The caller and the callee share the same runtime environment.

### What problem does "remote" solve?
As software systems grew larger, a single machine could not handle everything. Logic needed to be split across multiple machines — one machine might handle authentication, another handles database queries, another handles file storage.

The challenge: how does a program on Machine A ask a program on Machine B to execute some logic and return a result?

The naive approach is raw network programming — the developer on Machine A manually writes code to:
* Open a socket connection to Machine B
* Serialize the function name and arguments into bytes (if it is in java serilize in java, C in c way,...)
* Send those bytes over the network
* Wait for a response
* Deserialize the response bytes back into usable data
* Handle all network errors, timeouts, and retries

This is extremely repetitive across every function call that crosses a machine boundary. It also couples application logic tightly with low-level networking code.

RPC solves this by making a call to a remote procedure look and feel like a local procedure call. The developer writes `result = get_user(user_id)` and the RPC system handles all the networking underneath, invisibly.

### What is DCE?
DCE stands for Distributed Computing Environment. It is a set of standards and technologies developed in the early 1990s by the Open Software Foundation (OSF) — a consortium of companies including IBM, HP, and Digital Equipment Corporation (DEC).

DCE was designed to be a comprehensive, (IBM, Apple,.. each use their language)vendor-neutral framework for building distributed applications across heterogeneous systems (systems running different operating systems, different hardware architectures). It included:

* A naming/directory service (CDS — Cell Directory Service)
    - Computers cannot talk to each other if they cannot find each other. The CDS acts as a central registry.
    - Function: When a server offers a service (like a printer or a database), it registers its location in the CDS.
    - Usage: A client asks the CDS, "Where is the database service?" and the CDS provides the network address.
* A time synchronization service (DTS — Distributed Time Service)
    - In a distributed system, chronological order is vital for security and data logging.
    - The Challenge: Every computer’s internal clock drifts slightly.
    - The Solution: DTS synchronizes the clocks of all computers within the network to a millisecond level of accuracy. Without this, a security token might be marked as "expired" simply because one clock is two minutes faster than another.
* A security service (authentication and authorization)
    - DCE uses Kerberos-based authentication. It ensures three things:
    - Authentication: Proving the user is who they claim to be.
    - Authorization: Deciding if that user has permission to use a specific resource.
    - Integrity: Ensuring the message was not changed during transit.
* A threads service
* And most importantly: DCE RPC — the mechanism for making remote procedure calls

**The Structural Unit: The "Cell"**
DCE organizes resources into a group called a Cell. A Cell is a primary unit of administration that contains its own CDS, Security Service, and DTS.
* Internal Communication: Components within a cell trust each other by default via the local security server.
* External Communication: Cells can be linked together, allowing a user in "Cell A" to access a file in "Cell B" seamlessly.

DCE RPC became the foundation for many later technologies, most notably Microsoft's MSRPC, which in turn underpins DCOM, Active Directory, and parts of the Windows networking stack.

### How DCE RPC works, step by step
DCE RPC has several moving parts. Each one is introduced in order, from the most fundamental to the most operational.

#### 1. The Interface Definition Language (IDL)
Before any network communication can happen, both sides — the client (the caller) and the server (the callee) — must agree on:
* What procedures exist
* What arguments each procedure accepts
* What each procedure returns
* What data types are used

This agreement is written in IDL — Interface Definition Language. IDL is not a programming language for writing logic. It is a contract language — it describes the shape of an interface without implementing it(like c header file).

An IDL file defines an interface, which has:
* A unique identifier (UUID)
* A version number
* A list of procedure signatures (name, parameters, return type)
Example of what an IDL definition conceptually looks like:

```idl
interface UserService {
    [uuid("..."), version(1.0)]
    
    long GetUser([in] long user_id, [out] UserRecord *result);
    long DeleteUser([in] long user_id);
}
```
The `[in]` and `[out]` annotations are called directional attributes. They tell the RPC system which direction data flows:
- [in] — data goes from client to server
- [out] — data comes from server to client
- [in, out] — data goes both ways
This is critical for marshalling 

#### 2. The UUID (Universally Unique Identifier)
In a network with thousands of services, name collisions are inevitable. A Universally Unique Identifier (UUID) solves this by replacing human names with a mathematically unique 128-bit label. It looks like: `6bffd098-a112-3610-9833-46c3f87e345a`

UUIDs ensure that even if two different software vendors both create an interface called "UserService", they can coexist on the same network without collision. The UUID is the real identity of an interface; the human-readable name is just a label.

DCE standardized the UUID format. This same UUID concept later became the GUID (Globally Unique Identifier) in Microsoft's ecosystem — same idea, different name.

#### 3. The IDL Compiler and Stub Generation
Once the IDL file is written, it is fed into an IDL compiler (in DCE, this is called `dceidl` or similar). The compiler reads the interface definition and generates two pieces of code:

The IDL file is a source document used during the development phase. It is typically processed once on a developer's workstation using the IDL Compiler.
* Input: The service_definition.idl file.
* Output: The compiler generates source code files (the Client Stub and the Server Stub).
* Integration:
    - The Client Stub code is compiled into the Client Application.
    - The Server Stub code is compiled into the Server Application.
Once the applications are built, the IDL file and the compiler are no longer needed. The resulting programs are distributed to their respective machines.

1. **Client stub** — code that runs on the client side. It looks like a normal function to the rest of the client program, but internally it:
    * Takes the function arguments
    * Marshals them (serializes them into a standard byte format)
    * Sends them to the server over the network
    * Waits for the response
    * Unmarshals the response (deserializes the bytes back into return values)
    * Returns the result to the calling code as if nothing unusual happened

2. **Server stub** — code that runs on the server side. It:
    * Receives the incoming bytes from the network
    * Unmarshals the arguments
    * Calls the actual implementation of the procedure (written by the server developer)
    * Marshals the return value
    * Sends the response back to the client

#### 4. Marshalling and Unmarshalling
* Marshalling is the process of converting data (function arguments and return values) into a flat, transmittable byte sequence that can travel across a network.
* Unmarshalling is the reverse — converting those bytes back into structured data on the other end.

This is non-trivial because different machines may have:
* Different byte orderings (endianness — whether the most significant byte comes first or last)
* Different sizes for the same data type (an int might be 2 bytes on one system and 4 on another)
* Different struct alignment and padding rules

DCE RPC uses a standard called NDR — Network Data Representation for marshalling. NDR encodes data in a way that preserves meaning across different architectures. The receiving end knows how to interpret the bytes regardless of the sender's architecture.

NDR uses a concept called a transfer syntax — a description of how data is encoded on the wire.

#### 5. The Endpoint Mapper
On a server machine, many different RPC services may be running simultaneously. A client that wants to call `GetUser`(procedure) needs to know not just which machine to contact, but which port on that machine the UserService is listening on. (different service listen on different port)

DCE RPC solves this with the Endpoint Mapper (sometimes called portmapper or epmap). It is a well-known service that runs on a fixed, standardized port on every DCE server machine (port 135 in Microsoft's implementation).

The Endpoint Mapper maintains a registry of all currently registered RPC services — mapping each (UUID, version) pair to a specific network endpoint (IP + port or named pipe).

The flow:
1. The client contacts the Endpoint Mapper on port 135 of the target machine
2. The client sends the UUID and version of the interface it needs
3. The Endpoint Mapper looks up the registry and returns the actual port/endpoint where that service is listening
    - In the context of DCE RPC, an Endpoint does not refer to a different machine. Instead, it refers to a specific "doorway" or "address" inside a single machine.
    - An endpoint is the final destination for a network message within a host. In the DCE RPC framework, an endpoint is typically a combination of a protocol and a specific identifier:
        - TCP/UDP Endpoints: These are represented by Port Numbers (e.g., Port 49152).
        - Named Pipe Endpoints: These are used for communication between processes on the same machine or across a local network (e.g., `\pipe\lsarpc`).
4. The client then connects directly to that endpoint to make procedure calls

The Endpoint Mapper is essentially a directory service for RPC interfaces on a single machine.

#### 6. The Runtime Library
Both the client and server link against the DCE RPC runtime library. This is the library that actually implements all the network communication, connection management, error handling, and protocol negotiation.

The runtime is what the stubs call when they need to send or receive data. Developers don't interact with the runtime directly — it operates underneath the stubs.

The runtime handles:
* Establishing connections using the configured protocol sequence
* Fragmenting large messages into smaller packets if needed
* Reassembling fragments on the receiving end
* Detecting and reporting errors (network failure, timeout, server crash)
* Authentication and security (using DCE's security service)

#### 7. Protocol Sequences
DCE RPC is transport-independent — it can run over different underlying network protocols. The combination of a transport protocol and a network protocol is called a protocol sequence.

Common protocol sequences include:
* `ncacn_ip_tcp` — connection-oriented RPC over TCP/IP (most common)
* `ncadg_ip_udp` — datagram (connectionless) RPC over UDP/IP
* `ncacn_np` — RPC over named pipes (used heavily in Windows for intra-domain communication)
* `ncalrpc` — local RPC (on the same machine, bypasses the network entirely)

The server registers itself with the Endpoint Mapper specifying which protocol sequence it supports, and the client uses a matching protocol sequence to connect.

#### 8. Binding
Before a client can make an RPC call, it must establish a binding — a handle that represents the connection parameters between the client and a specific server endpoint.
A binding contains:
* The protocol sequence to use
* The network address of the server
* The endpoint (port or named pipe) on the server
* Any authentication/security context
There are two ways a binding is created:
1. Full binding (static) — the developer hardcodes the server address and endpoint directly. This is inflexible and only appropriate when the endpoint is fixed and known in advance.
2. Partial binding + Endpoint Mapper resolution (dynamic) — the developer provides only the server address and UUID. The runtime automatically contacts the Endpoint Mapper on that server to resolve the actual endpoint. This is the standard approach.

Once a binding is created, the client uses the binding handle as an argument (or implicitly, depending on the IDL) when making remote calls.

### The full call lifecycle
Putting everything together, here is the complete sequence of events for a single DCE RPC call:

#### Setup phase (happens once):
1. The server developer writes the IDL file defining the interface
2. The IDL compiler generates client and server stubs. Clinet application has to develop on top of the stub like wise server program has to develop on top of the server stub, these are knid of tightly coupled
    If any updation, Because the stubs are compiled directly into  application code:
    - The Server must be updated and redeployed to understand the new interface.
    - The Client must be updated and redeployed to call the new interface.
3. The server developer implements the actual procedure logic
4. The server starts, registers its interface with the local Endpoint Mapper (specifying UUID, version, protocol sequence, and dynamically-assigned port)

#### Call phase (happens for every remote call):
1. The client code calls a procedure by name — e.g., `GetUser(42, &record)`
2. The client stub intercepts this call
3. The client runtime contacts the Endpoint Mapper on the server machine (port 135) to resolve the endpoint for the target UUID
4. The client runtime establishes a connection to the resolved endpoint using the agreed protocol sequence
5. The client stub marshals the arguments using NDR into a byte buffer

6. The runtime sends the buffer over the network to the server

7. The server runtime receives the bytes and passes them to the server stub
8. The server stub unmarshals the arguments back into native data types
9. The server stub calls the actual implementation: `GetUser(42, &record)`
10. The implementation executes and returns a result
11. The server stub marshals the return value using NDR

12. The server runtime sends the response bytes back to the client

13. The client runtime receives the bytes
14. The client stub unmarshals the return value
15. The client stub returns the result to the calling code as a normal function return value

From the calling code's perspective: it called a function, waited briefly, and got a result back. The entire network exchange was invisible.

###  Security in DCE RPC
DCE RPC integrates with DCE's security infrastructure. Authentication ensures that the server knows who the client is, and authorization ensures the client is permitted to call the requested procedure.

DCE uses Kerberos as its authentication protocol. When a client binds to a server with authentication enabled:

1. The client obtains a Kerberos ticket for the target service from a Key Distribution Center (KDC)
2. The ticket is presented during binding
3. The server verifies the ticket cryptographically
4. A shared session key is established for message protection

DCE RPC supports several protection levels that can be negotiated:
1. RPC_C_PROTECT_LEVEL_NONE — no authentication
2. RPC_C_PROTECT_LEVEL_CONNECT — authenticate only at connection time
3. RPC_C_PROTECT_LEVEL_PKT — authenticate each packet
4. RPC_C_PROTECT_LEVEL_PKT_INTEGRITY — authenticate and verify integrity of each packet (detects tampering)
5. RPC_C_PROTECT_LEVEL_PKT_PRIVACY — authenticate, verify integrity, and encrypt the payload of each packet