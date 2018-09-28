## libsea - C Programming Made Easy

Libsea is a set of programming APIs designed to make many of the
common POSIX programming tasks less burdensome and complicated
for the programmer in C.

Includes APIs for:
 * Binary trees.
 * Hash tables.
 * Linked lists.
 * Binary and string buffers.
 * File system actions.
 * Inter-process communication.
 * TCP and TLS communication.
 * File system notification.
 * Process information.
 * Audio playback.
 * Server programming for TCP/IPv4, TCP/IPv6,and UNIX domain (TLS).
 * String conversion and manipulation.
 * System information (CPU, disks and mounts).
 * Thread programming (Threads, locks and spin locks).
 * HTTP/s requests.
 * Websocket (version 13) implementation.

Effort has been made to make these programming APIs portable and
currently libsea supports Linux, macOS, FreeBSD, DragonFlyBSD and
OpenBSD (all fully supported).

### Notes

This concept has been influenced by such works as the standard
Golang library. Work is non-funded and done in spare time. Any
reports or requests for features are happily accepted.

### Requirements

The library has two dependencies. OpenSSL/LibreSSL and SDL2.

There are no plans to introduce more dependencies. The reason
for this being simplicity.

### Installation

$ make (or gmake)
$ sudo make install

To install to a different location pass PREFIX to make when doing
a "make install", e.g:
	make PREFIX=/some/path install

### Documentation

You can generate documentation by running "doxygen" within the source
tree. If an API has no documentation the programmer can assume the
API is used internally or not stable.

### Authors

The library is maintained by Alastair Roy Poole with some work from
Sam Watkins.
