## Data translation
Data translation is the problem the presentation layer was originally designed to solve. It has two components: syntax conversion (the format of the data) and semantic preservation (the meaning of the data).

### Character encoding
Text is the most common type of data exchanged between networked applications. But text is not self-describing — a sequence of bytes is just numbers until a character encoding tells the receiver what each number means.

* **ASCII**:  Maps 128 characters to 7-bit codes. It covers the English alphabet, digits, punctuation, and control characters. ASCII is sufficient for English-only communication and is the historical baseline encoding of the internet. Any byte value from 0 to 127 means the same thing in ASCII on every machine.

* **Extended ASCII variants**: Is attempted to add more characters by using the 8th bit (values 128–255). This created dozens of incompatible encodings: ISO 8859-1 (Latin-1, used for Western European languages), ISO 8859-5 (Cyrillic), ISO 8859-6 (Arabic), and many others. A byte with value 200 means a different character in each of these encodings. Sending text between systems using different extended ASCII variants silently corrupts the content.

* **Unicode**: It resolved the incompatibility problem by defining a single universal character set that includes every character from every writing system — over 140,000 characters at present. Unicode assigns each character a code point, which is a unique number. Unicode is a **character set, not an encoding**; it defines what characters exist and what their code points are, but not how those code points are stored as bytes.
    - **UTF-8** is the most widely used encoding of Unicode on the internet. It encodes code points as sequences of one to four bytes. The key design property of UTF-8 is backward compatibility with ASCII: any byte below 128 is interpreted exactly as ASCII interprets it. This means all ASCII text is valid UTF-8 without modification. Characters outside the ASCII range use two, three, or four bytes. UTF-8 is the default encoding for HTML5, HTTP, JSON, XML, and most modern protocols.
    - **UTF-16** encodes most common characters as two bytes and uses four bytes for rarer characters. UTF-16 is used internally by Windows, Java, and JavaScript. It introduces a byte order issue: a two-byte code point can be stored high-byte-first or low-byte-first. UTF-16 files include a byte order mark (BOM, U+FEFF) at the beginning to indicate which byte order is in use.
    - **UTF-32** encodes every code point as exactly four bytes. This simplifies processing (character N is always at byte offset N × 4) at the cost of using four times as much space as ASCII for English text.

The presentation layer's job in character encoding is to know what encoding the sender used, convert the content to the agreed encoding for transmission, and ensure the receiver can decode it correctly. In HTTP, this is handled by the Content-Type header, which carries a charset parameter (e.g., Content-Type: text/html; charset=UTF-8).

### Byte order — big-endian versus little-endian
When a multi-byte value (an integer, a floating-point number, a Unicode code point in UTF-16) is stored in memory and then transmitted, the bytes can be arranged in two different orders.

* **Big-endian** stores the most significant byte first. The number `0x0A0B0C0D` is stored in memory as `0A 0B 0C 0D`. This is sometimes called network byte order because the original internet protocols standardized on big-endian for all multi-byte fields in headers.

* **Little-endian** stores the least significant byte first. The same number 0x0A0B0C0D is stored as `0D 0C 0B 0A`. x86 and x86-64 processors use little-endian.

The problem arises when a little-endian machine sends a raw multi-byte value to a big-endian machine, or vice versa, without any conversion. The receiver reads the bytes in the wrong order and computes a completely different number.

The presentation layer handles this by converting all multi-byte values to a canonical byte order before transmission. The canonical order for internet protocols is **big-endian** (network byte order). The C standard library provides `htonl` (host to network long), `htons` (host to network short), `ntohl`, and `ntohs` functions for this conversion. On a big-endian machine these are no-ops; on a little-endian machine they byte-swap the value.

Higher-level serialization formats (JSON, XML, Protocol Buffers) sidestep the byte order problem by encoding numbers as text or by always using a defined byte order, removing the need for explicit host-to-network conversion.

### Data serialization formats — the modern face of data translation
Serialization is the high-level process of converting complex data structures or objects into a flat format that can be stored in a file or sent over a network  and later reconstructed.

* **ASN.1** (Abstract Syntax Notation One) is the serialization standard defined as part of the OSI model and still used in TLS certificates, SNMP, and telephony protocols. ASN.1 defines a notation for describing data structures independently of any encoding. The encoding rules (how the abstract structure is converted to bytes) are defined separately — the most common are BER (Basic Encoding Rules), DER (Distinguished Encoding Rules, a strict subset of BER used in cryptography), and PER (Packed Encoding Rules, optimized for compactness). ASN.1 with DER encoding is what sits inside an X.509 TLS certificate.

* **XML** (Extensible Markup Language) encodes structured data as a hierarchy of tagged elements. XML is human-readable, self-describing (the tags carry both structure and semantic meaning), and language-neutral. It became the dominant data exchange format in enterprise systems during the 1990s and 2000s. XML documents include an optional declaration specifying the encoding (<?xml version="1.0" encoding="UTF-8"?>). XML is verbose — the same data takes significantly more bytes in XML than in binary formats.

* **JSON** (JavaScript Object Notation) encodes data as text using a simple structure: objects are `{key: value}` maps, arrays are `[value, value]` sequences, and values can be `strings, numbers, booleans, null, objects, or arrays`. JSON is less verbose than XML, easy to parse in almost every programming language, and has become the dominant format for REST APIs. JSON has no native binary type, no date type, and limited number precision (floating-point only, with precision limits). Extensions like BSON (Binary JSON, used by MongoDB) address the binary and type limitations.

* **Protocol Buffers** (protobuf) is a binary serialization format created by Google. Data structures are defined in `.proto` files using a schema language. The protobuf compiler generates encoding and decoding code for any target language. Protobuf messages are not self-describing — a raw protobuf byte stream cannot be interpreted without the schema. The advantage is compactness and speed: a protobuf message is typically three to ten times smaller than the equivalent JSON, and encoding/decoding is faster than text-based formats. Protobuf is the default serialization format for gRPC.

The choice of serialization format affects performance (binary formats are faster to encode and smaller), interoperability (text formats work in more contexts without tooling), and evolvability (schema-based formats handle version changes more gracefully).
