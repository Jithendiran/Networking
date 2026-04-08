## Encryption and decryption — protecting data in transit
The presentation layer is responsible for encrypting data before it leaves the sender and decrypting it when it arrives at the receiver. This ensures that anyone who intercepts the bytes in transit cannot read them.

### What encryption accomplishes

Encryption transforms plaintext (the readable original data) into ciphertext (an unreadable transformed version) using a mathematical algorithm and a key. The receiver, who holds the correct key, reverses the transformation to recover the plaintext. An attacker who intercepts the ciphertext and does not have the key cannot recover the plaintext in any practical amount of time, provided the algorithm and key are chosen correctly.

Encryption alone guarantees confidentiality — that only the intended receiver can read the data. It does not by itself guarantee integrity (that the data was not modified in transit) or authenticity (that the data actually came from the claimed sender). These additional guarantees require message authentication codes (MACs) or digital signatures, which are typically combined with encryption in the same protocol.

#### Symmetric encryption
Symmetric encryption uses the same key to encrypt and decrypt. Both the sender and receiver must possess the same secret key before communication begins.

**AES**
The dominant symmetric cipher in use today is `AES` (Advanced Encryption Standard). `AES` operates on 128-bit blocks of data and supports three key lengths: 128, 192, and 256 bits. AES with a `256-bit` key is considered computationally infeasible to break with any known attack on current or near-future hardware.

AES is a block cipher — it encrypts one fixed-size block at a time. To encrypt data of arbitrary length, a mode of operation is used to chain blocks together. The most important modes are:

* **CBC** (Cipher Block Chaining) XORs each plaintext block with the previous ciphertext block before encrypting. This removes patterns (identical plaintext blocks produce different ciphertext) but is not parallelizable. CBC requires a random initialization vector (IV) for the first block.

* **GCM** (Galois/Counter Mode) is an authenticated encryption mode: it encrypts the data and simultaneously produces an authentication tag that detects any modification to the ciphertext. GCM is parallelizable and is the preferred mode for AES in TLS 1.3 and most modern protocols. AES-256-GCM is a common specification that means AES with a 256-bit key in GCM mode.

* **ChaCha20-Poly1305** is an alternative to AES-GCM that is particularly efficient on devices without hardware AES acceleration (mobile CPUs, embedded systems). ChaCha20 is the stream cipher; Poly1305 provides authentication. TLS 1.3 supports both AES-GCM and ChaCha20-Poly1305.

The fundamental challenge of symmetric encryption is key distribution: how do the two sides agree on a shared secret key without an attacker learning it? If the key is sent over the network in plaintext before encryption begins, an attacker who intercepts it can decrypt all future messages. This problem is solved by asymmetric (public-key) cryptography.

#### Asymmetric encryption and key exchange
Asymmetric encryption uses two mathematically linked keys: a public key (which can be shared with anyone) and a private key (which is kept secret by its owner). Data encrypted with the public key can only be decrypted with the corresponding private key, and vice versa.

The most widely used asymmetric algorithm is RSA (Rivest-Shamir-Adleman). RSA's security rests on the difficulty of factoring large numbers: a public key contains the product of two large prime numbers, and breaking RSA requires factoring that product, which is computationally infeasible for sufficiently large key sizes (2048 bits or larger).

In practice, asymmetric encryption is not used directly to encrypt application data — it is far slower than symmetric encryption (by factors of 100 to 1000). Instead, it is used for key exchange: the two sides use asymmetric cryptography to agree on a shared symmetric key, then switch to symmetric encryption for the actual data.

> eavesdrop: to listen secretly to other people talking.

* **DHKE** (Diffie-Hellman Key Exchange) is the foundational algorithm for this purpose. Two parties exchange public values and independently compute the same shared secret, which an eavesdropper who sees only the public values cannot compute.
* **ECDH** (Elliptic Curve Diffie-Hellman) performs the same key exchange using elliptic curve mathematics. Elliptic curve algorithms achieve equivalent security with much shorter keys (256-bit ECDH is comparable to 3072-bit RSA). TLS 1.3 uses ECDH as its mandatory key exchange mechanism.

Forward secrecy (perfect forward secrecy) is a property where compromise of a long-term private key does not allow decryption of past sessions. It is achieved by generating fresh, ephemeral key pairs for each session's key exchange. The ephemeral keys are discarded after the session ends. Even if an attacker records all ciphertext and later obtains the server's long-term private key, they cannot decrypt past sessions because the ephemeral keys no longer exist. TLS 1.3 mandates forward secrecy — it only supports key exchange mechanisms (ECDHE, DHE) that provide this property.


#### Digital certificates and authentication
Encryption prevents eavesdropping but does not prevent a man-in-the-middle attack: an attacker who intercepts the connection could substitute their own public key for the server's and decrypt all traffic, while re-encrypting it for the real server. Neither side would know.

Authentication solves this. Before trusting a server's public key, the client needs evidence that the key actually belongs to the claimed server. This evidence takes the form of a digital certificate.

A digital certificate is a data structure containing:
* The server's domain name (or other identity)
* The server's public key
* The validity period (not before / not after dates)
* The issuing Certificate Authority (CA) and its digital signature over all of the above

The CA's signature is what creates trust. The CA's own public key is pre-installed in the operating system or browser (these are called root certificates or trust anchors). When a client receives a server's certificate, it verifies the CA's signature using the CA's pre-installed public key. If the signature is valid, the client knows the certificate was issued by a trusted CA, and therefore the public key in the certificate genuinely belongs to the named server.

The certificate for `google.com` itself is not in OS, but the "Master Key" (Root Certificate) used to verify it is.

Inside Operating System (Windows, macOS, Linux) or Browser (Firefox), there is a file called a Trust Store. This is a list of several hundred Root Certificates. These belong to organizations like DigiCert, Sectigo, or Google Trust Services. These certificates contain the Public Keys of these authorities. These are installed when install the OS or update browser.

What happens when `google.com` is searched?
Our computer does not yet have Google's specific certificate.
1. The Download: Google’s server sends its certificate to your browser.
2. The Signature Check: Google’s certificate contains a digital signature. To verify this signature, your browser needs a specific public key.
3. The Match: Your browser looks at the signature on Google's certificate and sees it was created by a specific CA (e.g., "Google Trust Services").
4. The Local Lookup: Your browser searches its local Trust Store (the pre-installed list) for the "Google Trust Services" Root Certificate.
5. Validation: Because the Root Certificate is already on your hard drive, the browser uses that pre-installed public key to mathematically verify that the signature on the google.com certificate is authentic.

The TLS handshake is the most common real-world implementation of the presentation layer's encryption responsibility. TLS 1.3 completes key exchange in one round trip (compared to two for TLS 1.2), and all server messages from Certificate onward are encrypted.

[Tls](./tls13_handshake_flow.svg)