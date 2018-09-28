#ifndef __NET_H__
#define __NET_H__

#include <openssl/ssl.h>

/**
 * @file
 * @brief Routines for TCP/IP connections (server/client).
 */

/**
 * @brief TCP/IP helpers.
 * @defgroup Net
 *
 * @{
 *
 * TCP/IP server/client helpers.
 */

typedef struct _tls_t
{
   BIO *bio;
   SSL_CTX *ctx;
} tls_t;

/**
 * Create a new TCP IPv4 listening socket.
 *
 * @param address The address of the server in string from.
 * @param port The port to listen on.
 *
 * @return The newly created listening socket file descriptor.
 */
int
net_tcp_server_new(const char *address, int port);

/**
 * Create a new TCP IPv6 listening socket.
 *
 * @param address The address of the server in string from.
 * @param port The port to listen on.
 *
 * @return The newly created listening socket file descriptor.
 */
int
net_tcp6_server_new(const char *address, int port);

/**
 * Connect to an IPv4 or IPv6 TCP location.
 *
 * @param address a semicolon delimited string containing the address and port of the remote host's service.
 *
 * @return The file descriptor of the connected socket.
 */
int
net_tcp_connect(const char *address);

/**
 * Obtain the address in human-readable form from an active TCP/IPv4 or IPv6 connection.
 *
 * @param sock The socket to obtain the address from.
 *
 * @return A newly created string of the remote or local address in human-readable form.
 */
char *
net_tcp_address_by_sock(int sock);

/**
 * Obtain the address in human-readable form from an active TCP/IPv6 connection.
 *
 * @param sock The socket to obtain the address from.
 *
 * @return A newly created string of the remote or local address in human-readable form.
 */
char *
net_tcp6_address_by_sock(int sock);

/**
 * Connect to IPv4 or IPv6 address using TLS.
 *
 * @param address a semi-colon delimited string containing the address and port of the remote host's service.
 *
 * @return Pointer to openssl BIO.
 */
tls_t *
net_tls_connect(const char *address);

/**
 * Close an active TCP/TLS connection.
 *
 * @param tls The TLS instance to close.
 */
void
net_tls_close(tls_t *tls);

/**
 * Close an active TCP/TLS connection and free the resource.
 *
 * @param tls The TLS instance to close and free.
 */
void
net_tls_free(tls_t *tls);

/**
 * Set timeout for TCP connection attempts.
 *
 * @param timeout The time in seconds for timeout on connect.
 */
void
net_tcp_connect_timeout_set(int timeout);

/**
 * Get the timeout for TCP connection attempts.
 *
 * @return Time in seconds.
 */
int
net_tcp_connect_timeout_get(void);

/**
 * Set timeout for TLS connection attempts.
 *
 * @param timeout The time in seconds for timeout on connect.
 */
void
net_tls_connect_timeout_set(int timeout);

/**
 * Get the timeout for TLS connection attempts.
 *
 * @return Time in seconds.
 */
int
net_tls_connect_timeout_get(void);

/**
 * @}
 */
#endif
