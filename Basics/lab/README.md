## Intro
In the Linux operating system, network interfaces (such as standard Ethernet devices or virtual TAP adapters) are not represented by conventional character or block device files within the `/dev` directory. Consequently, applications cannot obtain a direct file descriptor to these interfaces for executing ioctl() operations.

To bridge this gap, the Linux kernel architecture requires developers to instantiate a generic socket descriptor—typically utilizing the `AF_INET` domain and `SOCK_DGRAM` type—which serves as a handle to the network subsystem. Administrative configuration or diagnostic requests are then dispatched by passing this socket descriptor to `ioctl()`, coupled with a `struct ifreq` instance that explicitly specifies the target interface by name.
## Contents
1. [TAP](./TAP.md)