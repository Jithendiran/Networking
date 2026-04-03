## What SSL and TLS are and why they exist
The transport layer delivers bytes reliably between two endpoints. The session layer manages the conversation. The presentation layer translates and compresses data. But none of these layers, on their own, answer a fundamental question: 
* *can the two sides trust each other*
* *can they be certain that nobody else is reading or modifying what they exchange?*

Without a security layer, every byte sent over a network is readable by anyone who can observe the traffic — a router, an ISP, a compromised switch, or an attacker on the same Wi-Fi network. The bytes are also modifiable: an attacker in the middle of the path could change the content without either side knowing.

* **SSL** (Secure Sockets Layer) was created by Netscape in 1994 to solve this problem for web traffic. It was a protocol that sat between TCP and the application, wrapping the data in encryption before it crossed the network. SSL went through versions 1.0 (never publicly released), 2.0 (1995, quickly found to have serious vulnerabilities), and 3.0 (1996, a complete redesign).

* **TLS** (Transport Layer Security) is the successor to SSL. TLS 1.0 was published in 1999 as RFC 2246 and was essentially SSL 3.0 with fixes. TLS 1.1 followed in 2006, TLS 1.2 in 2008, and TLS 1.3 in 2018. SSL 2.0 and 3.0 are both deprecated and prohibited — no compliant implementation should accept them. TLS 1.0 and 1.1 are also deprecated as of 2021. The current standard is TLS 1.2 (still widely supported) and TLS 1.3 (the preferred version).

The name "SSL" persists in common usage even when the protocol in use is TLS. When someone says "SSL certificate" or "SSL connection," they almost always mean TLS.

## What TLS guarantees — the three security properties

TLS is designed to satisfy the CIA Triad (Confidentiality, Integrity, and Availability). It TLS provides exactly three guarantees

* **Confidentiality**. The data exchanged between the two sides *cannot be read* by any third party who observes the network traffic. Achieved by symmetric encryption — both sides agree on a shared secret key and encrypt all data with it.
    - Only you and the server can read the data. If a hacker intercepts it, it looks like gibberish.
* **Integrity**. The data *cannot be modified* in transit without detection. If an attacker changes even a single byte of the ciphertext, the receiver will detect the tampering and reject the data. Achieved by message authentication codes (MACs) or authenticated encryption (AEAD ciphers like AES-GCM).
    - The data hasn't been tampered with mid-flight. If a single bit changes, the connection drops.
* **Authentication**. At minimum, the client can verify that the server it is talking to is *genuinely the server* it intended to reach, not an impostor. Achieved by digital certificates and a chain of trust. Mutual TLS (mTLS) extends this so the server also verifies the client's identity.
    - You are actually talking to who you think you are (verified via Digital Certificates).

## Limitations

* TLS does not guarantee *anonymity* — the IP addresses of both sides are visible in the network headers, which are outside the TLS tunnel. 
    - TLS encrypts the content of your conversation, but it doesn’t hide the fact that you are talking.
    - Your ISP and any eavesdroppers on the network can still see your IP address and the server's IP address. They know you are visiting "BankOfAmerica.com," even if they can't see your account balance.
    - **Solution**: Anonymity will be resolved by *VPN* or *TOR*

* It does not guarantee *availability* — a TLS-protected server can still be taken offline by a denial-of-service attack. It guarantees exactly and only confidentiality, integrity, and authentication.
    - TLS lives at the Presentation/Session layers of the OSI model.
    - A DDoS (Distributed Denial of Service) attack usually targets the Network or Transport layers by flooding them with junk traffic. TLS can't stop a server from crashing because it's being overwhelmed by 10 million fake requests.
    - **Solution**: Availability will be resolved by *load balancers* and *DDoS mitigation*

## Components of TLS
> coherent: clear and easy to understand; logical.
> ephemeral: lasting or used for only a short period of time.

TLS is not a single invention. It assembles several existing cryptographic primitives into a coherent protocol. Each primitive solves one specific sub-problem.
1. **Symmetric ciphers** encrypt and decrypt data using a shared key. Fast enough to encrypt every byte of application data in real time. The shared key must be established before they can be used.
2. **Asymmetric** (public-key) cryptography allows two parties to establish a shared secret over an untrusted channel, and allows one party to prove its identity to the other. Slow — used only during the handshake, not for bulk data.
3. **Key derivation functions** (KDFs) take a shared secret (which may not be uniformly random) and derive cryptographically strong, independently random keys for different purposes (one key for client-to-server encryption, another for server-to-client, another for MACs, etc.).
    - When two computers connect via TLS, they first agree on a single, messy "raw ingredient" called a Pre-Master Secret. While this secret is private, it isn't formatted correctly to be used directly for encryption. It might be the wrong length, or it might have patterns that a hacker could exploit.
    - The KDF takes that raw secret and "stretches" it into multiple, high-quality keys. Here is how it works in professional, simple terms:
        1. Why?: "One Key is Never Enough"
            In professional security, we never use the same key for everything. If a hacker cracks one key, we don't want them to have the "skeleton key" to the entire house.

            A KDF ensures that even though we started with one secret, we end up with several independent keys:
            * Client Write Key: Used only for data going from client to the server. Server use this same key o decrypt, so this key is shared. why it is called as client write key is because, only client is allowed to write
            * Server Write Key: Used only for data coming from the server to client. The reason is same as above
            * MAC Keys: Used to ensure the data hasn't been tampered with (integrity).
        2. How?: "Extract and Expand"
            The KDF process usually follows two main steps:
            1. Extraction: It takes the "messy" shared secret and compresses it into a fixed-length, perfectly random string of bits. This removes any statistical weaknesses.
            2. Expansion: It takes that "perfect" string and stretches it out to produce as many keys as the session needs.
4. **Hash functions** produce a fixed-length digest of arbitrary-length input. Any change to the input produces a completely different digest. Used in MACs, digital signatures, and the key derivation process.
5. **Message Authentication Codes** (MACs) combine a hash function with a secret key to produce an authentication tag. The tag proves both that the message has not been modified and that it was produced by someone who holds the key.
    A MAC relies on two specific inputs to create a unique fingerprint: 
    1. A Secret Key: A piece of private information known only to the sender and the receiver.
    2. A Hash Function: A mathematical process that turns data into a fixed string of characters.
6. **Authenticated Encryption with Associated Data** (AEAD) combines encryption and authentication into a single operation. AES-GCM and ChaCha20-Poly1305 are AEAD ciphers — they produce ciphertext and an authentication tag in one pass, and decryption fails immediately if the tag does not match (meaning the ciphertext was tampered with).
7. **Digital signatures** use asymmetric keys to prove authenticity. A message signed with a *private key* can be verified by anyone holding the corresponding *public key*. Used by Certificate Authorities to sign certificates.
8. **Digital certificates** (X.509) bind a *public key* to an identity (a domain name, an organization) in a data structure that is signed by a trusted Certificate Authority.

## The TLS record layer — the basic unit of TLS
Before getting to the handshake, it is important to understand how TLS structures the data it sends. TLS organizes all communication into records. A record is a unit of data with a fixed header and a variable-length payload.

Every TLS record has a five-byte header:
* Content type (1 byte): what kind of data this record carries. The types are: `handshake (0x16)`, `change_cipher_spec (0x14, legacy)`, `alert (0x15)`, and `application_data (0x17)`.
* Protocol version (2 bytes): for TLS 1.3 records this is set to `0x0303` (which is the TLS 1.2 version number) for backward compatibility. The actual version is negotiated inside the handshake.
* Length (2 bytes): the number of bytes in the payload, maximum 16,384 bytes (16 KB).

After the header comes the payload — either plaintext (during the early handshake) or ciphertext (once encryption is established).

`TLS header + payload = TLS record layer`

TCP is a "stream," meaning it is just a continuous flow of bytes with no natural beginning or end. The Record Layer solves this by chopping that flow into structured blocks (records). Every message in TLS—whether it is a password, a "hello," or an error—must be placed inside one of these records before it can touch the TCP stream(from sender prespectivr).

### How Data Flows

The process follows a specific hierarchy:

**At the Sender (The Application Side)**
1. **Application Message**: The application (e.g., a web browser) produces raw data, such as an HTTP request.
2. **Sub-Protocol Identification**: This data is classified as Application Data (Type 0x17).
3. **Record Layer Packaging**: The Record Layer takes this raw data and performs three main tasks:
    1. Fragmentation: If the data is too large, it is broken into smaller chunks (max 16 KB).
    2. Security: If encryption is active, the Record Layer encrypts the data and adds a Message Authentication Code (MAC).
    3. Header Attachment: A 5-byte header is added to the front. This header specifies the content type, the version, and the exact length of the encrypted payload.
4. **Handover to TCP**: The completed TLS Record is handed to the TCP layer, which treats it as a simple stream of bytes to be sent across the internet.

**At the Receiver (The Server Side)**
1. **TCP Stream Reception**: The receiver gets a continuous stream of bytes from the network via TCP.
2. **Header Reading**: The receiver looks at the first 5 bytes to find the Length field. This tells the receiver exactly how many bytes to read from the stream to get the full "package."
3. **Record Processing**: Once the full record is collected, the Record Layer:
    - Verifies: It uses the MAC to ensure the data was not changed.
    - Decrypts: it turns the ciphertext back into readable plaintext.
4. **Delivery to Application**: Based on the Content Type in the header, the Record Layer knows this is "Application Data" and sends the cleaned-up message to the waiting application.

## Cipher suites — the negotiated set of algorithms
A cipher suite is a named combination of the cryptographic algorithms that TLS will use for a session. It names one algorithm for each role:`key exchange`, `authentication`, `bulk encryption` and `MAC`. Both sides must support the same cipher suite for communication to proceed.

In TLS 1.2, a cipher suite name encodes all four components. For example: `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256`

Reading left to right:
* `TLS` - the protocol
* `ECDHE` - key exchange algorithm (Elliptic Curve Diffie-Hellman Ephemeral)
* `RSA` - authentication algorithm 
* `AES_128_GCM` - bulk cipher (AES with 128-bit key in GCM mode — an AEAD cipher, so the MAC is implicit)
* `SHA256` - hash algorithm used in the PRF (pseudorandom function for key derivation)
In TLS 1.3, cipher suites are simplified. Because TLS 1.3 mandates ECDHE for key exchange and certificate-based authentication, those components are removed from the cipher suite name. The TLS 1.3 cipher suites name only the AEAD cipher and the hash function:
* `TLS_AES_256_GCM_SHA384`
* `TLS_AES_128_GCM_SHA256`
* `TLS_CHACHA20_POLY1305_SHA256`
During the ClientHello, the client sends a list of cipher suites it supports, in order of preference. The server picks the first one on the client's list that the server also supports. This is the agreed cipher suite for the session.

## The TLS 1.3 handshake — step by step
The TLS 1.3 handshake is the core of the protocol. It accomplishes three things simultaneously: key exchange (establishing the shared encryption key), authentication (verifying the server's identity), and cipher negotiation (agreeing on which algorithms to use). In TLS 1.3 this happens in one round trip.

### Step 1: ClientHello
The client sends the first message. It contains:
* Client random: 32 bytes of cryptographically random data. Used in key derivation.
* Session ID: a legacy field, present for compatibility with TLS 1.2 middleboxes.
* Cipher suites: a list of all cipher suites the client supports, in preference order.
* Extensions: where TLS 1.3 places its new functionality. The critical extensions are:
    - supported_versions: lists TLS versions the client supports. For TLS 1.3, this must include 0x0304. This is how the server knows the client supports TLS 1.3, despite the legacy version field in the record header saying 0x0303.
    - supported_groups: lists the elliptic curves (or finite field groups) the client supports for key exchange. Common values: x25519, secp256r1, secp384r1.
    - key_share: contains the client's ephemeral public key(s) for key exchange. The client includes a key share for its most preferred group so the server can immediately use it without another round trip.
    - signature_algorithms: lists the signature schemes the client will accept for certificate authentication.
    - server_name (SNI — Server Name Indication): tells the server which hostname the client is trying to reach. This allows a single server with one IP address to host multiple domains and present the correct certificate for each.

### Step 2: ServerHello
The server responds. It contains:
* Server random: 32 bytes of cryptographically random data.
* Session ID: echoes the client's session ID.
* Cipher suite: the single cipher suite the server has chosen from the client's list.
* Extensions:
    - supported_versions: confirms 0x0304 (TLS 1.3).
    - key_share: the server's ephemeral public key for the chosen group. Combined with the client's key share, both sides can now independently compute the same shared secret using ECDH mathematics.

At this point — after ServerHello — both sides have everything needed to compute the *shared secret*. The ECDH computation happens immediately, and from this point forward all subsequent messages are *encrypted*.

#### Key derivation — from shared secret to working keys
The raw ECDH shared secret is not used directly as an encryption key. It is processed through a key derivation function to produce multiple independent keys, each for a specific purpose.

TLS 1.3 uses HKDF (HMAC-based Key Derivation Function) with a specific schedule called the key schedule. The inputs to the key schedule are the ECDH shared secret and a series of transcript hashes — hashes of all the handshake messages seen so far. The transcript hash binds the derived keys to the exact messages exchanged, so if any handshake message was tampered with, the derived keys will be wrong and decryption will fail.

The key schedule produces several distinct secrets:
- Handshake traffic secrets: used to encrypt the remaining handshake messages (EncryptedExtensions, Certificate, CertificateVerify, Finished).
- Application traffic secrets: used to encrypt application data after the handshake completes. The client and server have separate application traffic secrets (one for client-to-server, one for server-to-client) so there is no key reuse between directions.
- Resumption master secret: used to derive session tickets for future 0-RTT connections.

Each traffic secret is expanded into an actual key and an IV (initialization vector) for the AEAD cipher.

### Step 3: EncryptedExtensions
The first encrypted message from the server. It contains extensions that do not need to be in plaintext — for example, indicating that the server requires client certificate authentication (for mTLS). In TLS 1.2, these extensions were sent in plaintext. Encrypting them in TLS 1.3 prevents a passive observer from learning details about the connection beyond what is visible in ClientHello and ServerHello.

### Step 4: Certificate
The server sends its X.509 certificate chain. This is the server's proof of identity. The chain typically contains:
1. The server's own certificate, including its public key and domain name.
2. One or more intermediate CA certificates.

The root CA certificate is not sent — the client is expected to have it pre-installed in its trust store. Sending the root would waste bandwidth since the client can only trust pre-installed roots anyway.

In TLS 1.3, the Certificate message is encrypted. In TLS 1.2, it was sent in plaintext, meaning a passive observer could see the server's certificate (and therefore the server's domain name) even without breaking encryption.

### Step 5: CertificateVerify
The server proves that it actually holds the private key corresponding to the public key in its certificate. It does this by creating a digital signature over a hash of the entire handshake transcript so far, using its private key.

The client verifies this signature using the server's public key (extracted from the certificate). If the signature is valid, the client knows: the server has the private key, the public key in the certificate is genuine, and the certificate was not substituted mid-handshake (because the signature covers the transcript).

This is the authentication step. Without it, a man-in-the-middle attacker could substitute their own certificate.

### Step 6: Finished (server)
The server sends a Finished message — an HMAC over the entire handshake transcript, computed using the handshake traffic secret. This authenticates the handshake at the MAC level. If any message was tampered with, this MAC will not verify.

### Step 7: Finished (client)
The client verifies the server's Finished message, then sends its own Finished message (same structure, from the client's perspective). After the server verifies the client's Finished message, both sides switch to application traffic keys.

The handshake is complete. Application data may now flow in both directions, encrypted and authenticated with the application traffic keys.

