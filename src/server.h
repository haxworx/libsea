#ifndef __SERVER_H__
#define __SERVER_H__

/**
 * @file
 * @brief Create a new TCP or UNIX domain socket server.
 */

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/**
 * @brief Creation, management and manipulation of a server.
 * @defgroup Server
 *
 * @{
 *
 * Creation, management and manipulation of a server.
 */

#define CLIENT_TIMEOUT_DEFAULT 60 * 5

typedef enum
{
   ERR_SOCKET_FAILED = (1 << 0),
   ERR_SETSOCKOPT_FAILED = (1 << 1),
   ERR_BIND_FAILED = (1 << 2),
   ERR_LISTEN_FAILED = (1 << 3),
   ERR_SELECT_FAILED = (1 << 4),
   ERR_ACCEPT_FAILED = (1 << 5),
   ERR_READ_FAILED = (1 << 6),
} error_network_t;

typedef enum
{
   CLIENT_STATE_IGNORE = -3,
   CLIENT_STATE_CONTROL_FRAME = -2,
   CLIENT_STATE_ERROR = -1,
   CLIENT_STATE_DISCONNECT = 0,
   CLIENT_STATE_CONNECTED = 1,
   CLIENT_STATE_READ_CONTINUE = 2,
   CLIENT_STATE_DEFAULT = 3,
} client_state_t;

typedef enum
{
   CLIENT_DATA_TYPE_TEXT = 0,
   CLIENT_DATA_TYPE_BINARY = 1,
} recv_data_t;

typedef enum
{
   SERVER_EVENT_CALLBACK_ADD,
   SERVER_EVENT_CALLBACK_DEL,
   SERVER_EVENT_CALLBACK_DATA,
   SERVER_EVENT_CALLBACK_ERR,
} server_callback_t;

typedef struct server_t server_t;
typedef struct server_event_t server_event_t;

typedef struct _data_t
{
   char *data;
   long int size;
   bool is_continued;
   recv_data_t type;
} _data_t;

typedef _data_t server_data_t;
typedef _data_t client_data_t;

typedef struct server_client_t server_client_t;
struct server_client_t {
   int sock;
   SSL *ssl;
   struct pollfd *pfd;

   server_t *server;
   bool is_websocket;
   client_state_t state;
   int masking_key;
   int64_t unixtime;
   int64_t ping_value;
   client_data_t received;
   server_client_t *next;
};

struct server_event_t
{
   server_callback_t type;
   server_t         *server;
   server_client_t  *client;
   server_data_t    *received;
};

typedef int (*callback_fn)(server_event_t *event, void *data);

struct server_t
{
   int            sock;
   int            socket_count;
   int            sockets_max;

   int            protocol;
   char          *address;
   char          *local_address;
   uint16_t       port;

   bool           is_websocket;

   bool           is_tls;
   SSL_CTX       *ctx;
   char          *cert;
   char          *private_key;
   char          *private_key_password;

   bool           enabled;
   time_t         client_timeout;

   struct pollfd *sockets;
   int            poll_array_size;
   server_client_t      **clients;
   /* Callbacks */

   callback_fn  client_add_cb;
   callback_fn  client_recv_cb;
   callback_fn  client_del_cb;
   callback_fn  client_err_cb;

   void         *client_add_data;
   void         *client_recv_data;
   void         *client_del_data;
   void         *client_err_data;
};

/**
 * Create a new server instance.
 *
 * @return A pointer to the newly created server object.
 */
server_t
*server_new(void);

/**
 * Set the server's protocol, address and port by a string.
 *
 * @param server The server object to manipulate.
 * @param address The string containing a valid HOST:PORT (TCP/IP) or file:// (unix domain) location to listen on.
 *
 * @return Non-zero on success or zero on failure.
 */
int
server_config_address_port_set(server_t *server, const char *address);

/**
 * Set which address to listen on (TCP/IP).
 *
 * @param server The server object to manipulate.
 * @param address The human-readable address to listen on.
 */
void
server_config_address_set(server_t *server, const char *address);

/**
 * Set listening port for server to listen on (TCP/IP).
 *
 * @param server The server object.
 * @param port The port to listen on.
 */
void
server_config_port_set(server_t *server, int port);

/**
 * Enable IPv6 if server is TCP/IP.
 *
 * @param server The server object.
 * @param enabled Enable IPv6 if non-zero.
 */
void
server_config_ipv6_enable_set(server_t *server, int enabled);

/**
 * Set maximum number of clients server can handle.
 *
 * @param server The server object.
 * @param max The maximum number of concurrent clients.
 */
void
server_config_clients_max_set(server_t *server, int max);

/**
 * Set a timeout value for client inactivity and disconnect client.
 *
 * @param server The server object.
 * @param seconds The time in seconds to timeout from inactivity.
 */
void
server_config_client_timeout_set(server_t *server, time_t seconds);

/**
 * Free the server and all resources associated with it.
 *
 * @param server The server object to free.
 */
void
server_free(server_t *server);

/**
 * Start the server's main loop.
 *
 * @param server The server object.
 */
void
server_run(server_t *server);

/**
 * Instruct the server to terminate gracefully.
 *
 * @param server The server object to terminate/shutdown.
 */
void
server_terminate(server_t *server);

/**
 * Specify a callback and event type and associate them.
 *
 * @param server The server object.
 * @param type The event type.
 * @param func The function to be called on event.
 * @param data User data to be passed to the callback associated with this event.
 */
void
server_event_callback_set(server_t *server, server_callback_t type, int (*func)(server_event_t *event, void *data), void *data);

/**
 * Write data to a client connected to the server.
 *
 * @param client The client to send data to.
 * @param buf The data to send.
 * @param len The length of data in bytes to send to the client.
 *
 * @return On error will return -1 otherwise the number of bytes written.
 */
int
server_client_write(server_client_t *client, const char *buf, size_t len);

/**
 * Delete a client from a server.
 *
 * @param server The server object.
 * @param client The client to disconnect and delete from the server instance.
 */
void
server_client_del(server_t *server, server_client_t *client);

/**
 * Get a human-readable address of the remote client. Can be TCP/IPv4 or IPv6 or UNIX domain.
 *
 * @param client The client to query.
 *
 * @return A newly allocated NULL terminated string containing a human-readable address.
 */
char *
server_client_address_get(server_client_t *client);

/**
 * Enable TLS communication on server.
 *
 * @param enabled True or false.
 *
 */
void
server_tls_enabled_set(server_t *server, bool enabled);

/**
 * Set TLS certificate and private key.
 *
 * @param server The server object.
 * @param cert The location of the certificate file on disk.
 * @param private_key The location of the private key file on disk.
 *
 */
void
server_tls_keys_set(server_t *server, const char *cert, const char *private_key);

/**
 * Set TLS private key password.
 *
 * @param server The server object.
 * @param password The password for the TLS private key.
 *
 */
void
server_tls_private_key_password_set(server_t *server, const char *password);

/**
 * Enable ws communication.
 *
 * @param server The server object.
 * @param enabled True of  false.
 */
void
server_websocket_enabled_set(server_t *server, bool enabled);

/**
 * Check whether data is text.
 *
 * @param data Data received.
 *
 * @return True or false.
 */
bool
server_received_is_text(server_data_t *data);

/**
 * Check whether data is binary.
 *
 * @param data Data received.
 *
 * @return True or false.
 */
bool
server_received_is_binary(server_data_t *data);

/**
 * @}
 */
#endif
