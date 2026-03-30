## Compression and decompression

Compression reduces the number of bytes that must be transmitted. Less data takes less time to send and consumes less bandwidth. The presentation layer applies compression to data after translation but before encryption.

### How lossless compression works conceptually
Compression exploits redundancy. If the same sequence of bytes appears many times in a file, it can be replaced by a short reference the first time it appears and then a pointer to that reference on subsequent occurrences. Text is highly compressible because natural language is repetitive — common words, phrases, and patterns appear over and over.

Deflate is the algorithm underlying gzip and zlib. It combines two techniques: LZ77 (which replaces repeated sequences with back-references) and Huffman coding (which assigns shorter bit sequences to more frequent symbols). Deflate is widely used for HTTP content encoding.

* **gzip** wraps deflate with a header and checksum. The HTTP Content-Encoding: gzip header tells the client that the response body is gzip-compressed. The client decompresses before passing the content to the application. gzip typically reduces HTML, JSON, and CSS by 60–80% compared to uncompressed.

* **Brotli** is a newer compression algorithm from Google, designed specifically for HTTP content. Brotli achieves better compression ratios than gzip, especially on text, by using a pre-defined dictionary of common strings from web content alongside the standard LZ and Huffman techniques. HTTP clients advertise Brotli support with Accept-Encoding: br; servers respond with Content-Encoding: br.

* **zstd** (Zstandard) is a compression algorithm from Facebook that combines high compression ratio with fast decompression speed. It is used in databases, filesystems, and increasingly in network protocols (HTTP, QUIC).

* **LZMA** (used in 7-zip and XZ archives) achieves very high compression ratios at the cost of slow compression speed. It is suited for archives and software distribution where compression happens once but decompression happens many times.

### Compression and encryption interaction

As stated earlier, compression must happen before encryption. The reason is mathematical: after encryption, the byte stream has high entropy — every byte value appears with roughly equal probability and there are no exploitable patterns. Compression algorithms depend entirely on patterns. Compressing high-entropy data produces output that is the same size or larger than the input, wasting processing time without benefit.


**Image and media compression**
Not all compression is lossless. For images, audio, and video, lossy compression discards information that human perception does not detect, achieving much greater size reduction than lossless techniques.

* `JPEG` uses lossy compression for photographs by discarding high-frequency detail in each image block. The compression level is tunable — higher compression means more detail loss.

* `PNG` uses lossless compression (deflate) and is the correct choice for images with sharp edges, text, or areas of flat color (logos, screenshots, diagrams).

* `WebP` is a format from Google that supports both lossy and lossless compression and achieves smaller file sizes than JPEG or PNG for comparable quality.

There are many formats. The presentation layer handles these formats at the metadata and negotiation level — agreeing on which format is in use (e.g., via MIME types in HTTP), ensuring the sender and receiver both support it, and framing the compressed data correctly so the receiver can decode it.

### MIME types — naming data formats at the presentation boundary
A MIME type (Multipurpose Internet Mail Extensions type, now more broadly called a media type) is a standardized string that identifies the format of a piece of data. It tells the receiver what kind of data it is receiving and how to interpret it.

A MIME type has the form type/subtype, optionally followed by parameters. Examples:
```
text/html; charset=UTF-8 — HTML document, encoded in UTF-8
application/json — JSON data
application/octet-stream — arbitrary binary data, format unspecified
image/jpeg — JPEG image
audio/mpeg — MP3 audio
video/mp4 — MP4 video
application/protobuf — Protocol Buffers serialized data
application/xml — XML document
multipart/form-data — form submission containing multiple parts
```
MIME types are registered with IANA (Internet Assigned Numbers Authority). Vendors can also use types of the form `application/vnd.vendor.format` for proprietary formats.

In HTTP, MIME types appear in Content-Type (describing what is being sent) and in Accept headers (describing what the client is willing to receive). The server uses the Accept header to perform content negotiation: choosing which representation of the data to send based on the client's stated preferences. This is a direct expression of the presentation layer's translation responsibility — the same underlying data may be sent as JSON, XML, or protobuf depending on what the client accepts.
