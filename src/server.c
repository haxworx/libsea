#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include "ipc.h"
#include "net.h"
#include "server.h"
#include "websocket.h"
#include "errors.h"

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
# define TLS_server_method TLSv1_2_server_method
#endif

static server_client_t *
_clients_add(server_t *server, int sock, SSL *ssl)
{
   server_client_t **clients;
   server_client_t *tmp, *c;
   struct pollfd *pfd, *sockets;
   int socket_index = 0;

   sockets = server->sockets;
   clients = server->clients;

   for (int i = 1; i < server->sockets_max; i++)
     {
        if (sockets[i].fd == -1)
          {
             socket_index = i;
             break;
          }
     }

   if (socket_index == 0)
     {
        close(sock);
        return NULL;
     }

#if defined(__MACH__) && defined(__APPLE__)
   int on;
   setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif

   server->socket_count++;

   pfd = &sockets[socket_index];
   sockets[socket_index].fd = sock;
   sockets[socket_index].events = POLLIN;

   tmp = calloc(1, sizeof(server_client_t));
   if (!tmp)
     return NULL;

   tmp->sock = sock;
   tmp->ssl = ssl;
   tmp->pfd = pfd;
   tmp->is_websocket = server->is_websocket;
   tmp->unixtime = time(NULL);

   c = clients[0];
   if (c == NULL)
     {
        clients[0] = c = tmp;

        return c;
     }

   while (c->next)
     c = c->next;

   if (c->next == NULL)
     {
        c->next = tmp;
     }

   return tmp;
}

static void
_client_data_free(server_client_t *client)
{
   if (!client->received.data)
     return;

   free(client->received.data);

   client->received.data = NULL;
}

static void
_clients_del(server_t *server, server_client_t *client)
{
   server_client_t **clients;
   server_client_t *c, *prev;
   struct pollfd *sockets;

   sockets = server->sockets;

   clients = server->clients;

   server->socket_count--;

   for (int i = 1; i < server->sockets_max; i++)
     {
        if (sockets[i].fd == client->sock)
          {
             sockets[i].fd = -1;
             break;
          }
     }

   if (client->ssl)
     SSL_free(client->ssl);

   client->ssl = NULL;

   close(client->sock);

   client->pfd->fd = -1;

   prev = NULL;
   c = clients[0];
   while (c)
     {
        if (c == client)
          {
             if (prev)
               {
                  prev->next = c->next;
               }
             else
               {
                  clients[0] = c->next;
               }

             _client_data_free(c);
             free(c);
             c = NULL;

             return;
          }
        prev = c;
        c = c->next;
     }
}

static server_client_t *
_client_by_fd(server_client_t **clients, int fd)
{
   server_client_t *c = clients[0];
   while (c)
     {
        if (c->sock == fd)
          {
             return c;
          }
        c = c->next;
     }

   return NULL;
}

ssize_t
server_client_read(server_client_t *client, char *buf, size_t len)
{
   if (client->ssl)
     return SSL_read(client->ssl, buf, len);

   return read(client->sock, buf, len);
}

static int
_client_request(server_client_t *client, char *buf, ssize_t bytes)
{
   client->unixtime = time(NULL);

   client->received.type = CLIENT_DATA_TYPE_TEXT;
   client->received.size = bytes;
   client->received.data = malloc(1 + client->received.size * sizeof(char));

   memcpy(client->received.data, buf, client->received.size);
   client->received.data[client->received.size] = '\0';
   client->state = CLIENT_STATE_DEFAULT;

   return bytes;
}

static int
_client_read(server_client_t *client)
{
   char buf[65535];
   ssize_t bytes;

   _client_data_free(client);

   do
     {
        bytes = server_client_read(client, buf, sizeof(buf) -1);
        if (bytes == 0)
          {
             return CLIENT_STATE_DISCONNECT;
          }
        else if (bytes < 0)
          {
             if (client->ssl)
               {
                  return CLIENT_STATE_DISCONNECT;
               }

             switch (errno)
               {
                case EAGAIN:
                case EINTR:
                  return bytes;

                case ECONNRESET:
                  return CLIENT_STATE_DISCONNECT;

                default:
                  exit(ERR_READ_FAILED);
               }
          }
     } while (0);

   buf[bytes] = 0x00;

   return _client_request(client, buf, bytes);
}

static void
_clients_free(server_client_t **clients)
{
   server_client_t *next, *c = clients[0];

   while (c)
     {
        next = c->next;
        _client_data_free(c);
        if (c->ssl)
          SSL_free(c->ssl);

        close(c->sock);

        free(c);
        c = next;
     }

   free(clients);
}

static void
_on_add_cb(server_t *server, server_client_t *client)
{
   if (server->client_add_cb)
     {
        server_event_t *ev = calloc(1, sizeof(server_event_t));
        ev->type = SERVER_EVENT_CALLBACK_ADD;
        ev->client = calloc(1, sizeof(server_client_t));
        ev->client->sock = client->sock;
        ev->client->ssl = client->ssl;
        ev->client->server = server;
        ev->server = server;

        server->client_add_cb(ev, server->client_add_data);

        free(ev->client);
        free(ev);
     }
}

static void
_on_data_cb(server_t *server, server_client_t *client)
{
   if (server->client_recv_cb)
     {
        server_event_t *ev = calloc(1, sizeof(server_event_t));
        ev->type = SERVER_EVENT_CALLBACK_DATA;
        ev->client = calloc(1, sizeof(server_client_t));
        ev->client->sock = client->sock;
        ev->client->ssl = client->ssl;
        ev->client->server = server;
        ev->server = server;
        ev->received = calloc(1, sizeof(server_data_t));
        ev->received->data = client->received.data;
        ev->received->size = client->received.size;
        ev->received->type = client->received.type;

        server->client_recv_cb(ev, server->client_recv_data);

        free(ev->client);
        free(ev->received);
        free(ev);
     }
}

static void
_on_del_cb(server_t *server, server_client_t *client)
{
   if (server->client_del_cb)
     {
        server_event_t *ev = calloc(1, sizeof(server_event_t));
        ev->type = SERVER_EVENT_CALLBACK_DEL;
        ev->client = calloc(1, sizeof(server_client_t));
        ev->client->sock = client->sock;
        ev->client->ssl = client->ssl;
        ev->client->server = server;
        ev->server = server;

        server->client_del_cb(ev, server->client_del_data);

        free(ev->client);
        free(ev);
     }
}

static void
_on_err_cb(server_t *server, server_client_t *client)
{
   if (server->client_err_cb)
     {
        server_event_t *ev = calloc(1, sizeof(server_event_t));
        ev->type = SERVER_EVENT_CALLBACK_ERR;
        ev->client = calloc(1, sizeof(server_client_t));
        ev->client->sock = client->sock;
        ev->client->ssl = client->ssl;
        ev->client->server = server;
        ev->server = server;

        server->client_err_cb(ev, server->client_err_data);

        free(ev->client);
        free(ev);
     }
}

static void
_clients_timeout_check(server_t *server)
{
   server_client_t **clients;
   server_client_t *c;

   clients = server->clients;
   c = clients[0];

   while (c)
     {
        if (c->unixtime < (time(NULL) - server->client_timeout))
          {
             _on_del_cb(server, c);
             _clients_del(server, c);
             c = clients[0];
             continue;
          }
        c = c->next;
     }
}

int
server_client_write(server_client_t *client, const char *buf, size_t len)
{
   server_t *server;

   server = client->server;

   if (server->is_websocket)
     return ws_client_write(client, buf, len);

   if (client->ssl)
     return SSL_write(client->ssl, buf, len);

   return send(client->sock, buf, len, MSG_NOSIGNAL);
}

char *
server_client_address_get(server_client_t *client)
{
   if (client->server->protocol == AF_INET || client->server->protocol == AF_INET6)
     {
        return net_tcp_address_by_sock(client->sock);
     }

   return ipc_unix_address_by_sock(client->sock);
}

bool
server_received_is_text(server_data_t *recv)
{
   if (recv->type == CLIENT_DATA_TYPE_TEXT)
     return true;

   return false;
}

bool
server_received_is_binary(server_data_t *recv)
{
   if(recv->type == CLIENT_DATA_TYPE_BINARY)
     return true;

   return false;
}

void
server_websocket_enabled_set(server_t *server, bool enabled)
{
   server->is_websocket = enabled;
}

void
server_tls_enabled_set(server_t *server, bool enabled)
{
   server->is_tls = enabled;
}

void
server_tls_keys_set(server_t *server, const char *cert, const char *private_key)
{
   server->cert = strdup(cert);
   server->private_key = strdup(private_key);
}

void
server_tls_private_key_password_set(server_t *server, const char *password)
{
   server->private_key_password = strdup(password);
}

int
server_config_address_port_set(server_t *server, const char *address)
{
   char *port_s, *ip;
   int port;
   struct addrinfo hint, *res = NULL;

   if (!address || !address[0])
     return 0;

   if (!strncasecmp(address, "file://", 7))
     {
        ip = (char *) address + 7;
        if (!ip || !ip[0]) return 0;
        server->protocol = AF_UNIX;
        server_config_address_set(server, ip);
        return 1;
     }

   memset(&hint, 0, sizeof(hint));
   hint.ai_family = PF_UNSPEC;
   hint.ai_flags = AI_NUMERICHOST;

   ip = strdup(address);
   port_s = strrchr(ip, ':');
   if (!port_s) return 0;
   *port_s = '\0';
   port_s++;
   if (!port_s) return 0;

   port = atoi(port_s);
   if (port < 1 || port > 65535)
     return 0;

   server_config_port_set(server, port);

   if (!getaddrinfo(ip, NULL, &hint, &res))
     {
        if (res->ai_family == AF_INET)
          {
             server->protocol = AF_INET;
             server_config_address_set(server, ip);
          }
        else if (res->ai_family == AF_INET6)
          {
             server->protocol = AF_INET6;
             server_config_address_set(server, ip);
          }
     }
   else if (!strcmp(ip, "*"))
     {
        server->protocol = AF_INET6;
        server_config_address_set(server, ip);
     }
   else
     {
        return 0;
     }

   free(ip);

   if (res)
     freeaddrinfo(res);

   return 1;
}

void
server_config_client_timeout_set(server_t *server, time_t seconds)
{
   server->client_timeout = seconds;
}

void
server_config_port_set(server_t *server, int port)
{
   if ((port > 0) || (port <= 65535))
     {
        server->port = port;
     }
}

void
server_config_address_set(server_t *server, const char *address)
{
   char buf[256];

   if (server->protocol == AF_UNIX)
     {
        server->local_address = strdup(address);
     }
   else
     {
        snprintf(buf, sizeof(buf), "%s:%d", address, server->port);
        server->local_address = strdup(buf);
     }

   server->address = strdup(address);
}

void
server_config_clients_max_set(server_t *server, int max)
{
   void *tmp;
   int i, prev;

   if (max > server->poll_array_size)
     {
        prev = server->poll_array_size;
        server->poll_array_size = max + 1;

        tmp = realloc(server->sockets, server->poll_array_size * sizeof(struct pollfd));
        if (!tmp)
          errors_fail("realloc: server_config_clients_max_set()");

        server->sockets = tmp;
        server->sockets_max = server->poll_array_size;

        for (i = prev; i < server->poll_array_size; i++)
          {
             server->sockets[i].fd = -1;
          }

        return;
     }

   server->sockets_max = max + 1;
}

void
server_config_ipv6_enable_set(server_t *server, int enabled)
{
   if (enabled)
     server->protocol = AF_INET6;
   else
     server->protocol = AF_INET;
}

void
server_event_callback_set(server_t *server, server_callback_t type,
                          int (*func)(server_event_t *, void *data), void *data)
{
   switch (type)
     {
      case SERVER_EVENT_CALLBACK_ADD:
        server->client_add_cb = func;
        server->client_add_data = data;
        break;

      case SERVER_EVENT_CALLBACK_DEL:
        server->client_del_cb = func;
        server->client_del_data = data;
        break;

      case SERVER_EVENT_CALLBACK_DATA:
        server->client_recv_cb = func;
        server->client_recv_data = data;
        break;

      case SERVER_EVENT_CALLBACK_ERR:
        server->client_err_cb = func;
        server->client_err_data = data;
        break;
     }
}

void
server_client_del(server_t *server, server_client_t *client)
{
   server_client_t *tmp = _client_by_fd(server->clients, client->sock);
   if (!tmp) return;

   _on_del_cb(server, tmp);
   _clients_del(server, tmp);
}

void
server_free(server_t *server)
{
   struct pollfd *sockets = server->sockets;

   for (int i = 0; i < server->sockets_max; i++)
     {
        if (sockets[i].fd != -1)
          close(sockets[i].fd);
     }

   _clients_free(server->clients);

   if (server->ctx)
     SSL_CTX_free(server->ctx);

   if (server->cert)
     free(server->cert);

   if (server->private_key)
     free(server->private_key);

   if (server->private_key_password)
     free(server->private_key_password);

   if (server->address)
     free(server->address);

   if (server->local_address)
     free(server->local_address);

   if (server->protocol == AF_UNIX)
     ipc_unix_destroy(server->sock);
   else
     close(server->sock);

   free(server->sockets);
   free(server);
}

void
server_terminate(server_t *server)
{
   server->enabled = false;
}

static void
_tls_init(void)
{
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();
}

static SSL_CTX *
_tls_create(server_t *server)
{
   const SSL_METHOD *method;
   SSL_CTX *ctx;
   method = TLS_server_method();
   ctx = SSL_CTX_new(method);
   if (!ctx)
     return NULL;

    /* Set the key and cert */
   if (SSL_CTX_use_certificate_file(ctx, server->cert, SSL_FILETYPE_PEM) <= 0)
     exit(EXIT_FAILURE);

   if (server->private_key_password)
     SSL_CTX_set_default_passwd_cb_userdata(ctx, server->private_key_password);

   if (SSL_CTX_use_PrivateKey_file(ctx, server->private_key, SSL_FILETYPE_PEM) <= 0 )
     exit(EXIT_FAILURE);

   return ctx;
}

void
_server_accept(server_t *server)
{
   struct sockaddr_in clientname;
   int sock, flags, ret = -1;
   socklen_t size;

   do
     {
        SSL *ssl = NULL;
        size = sizeof(clientname);
        sock = accept(server->sock, (struct sockaddr *)&clientname, &size);
        if (sock < 0)
          {
             if (errno == EAGAIN || errno == EWOULDBLOCK ||errno == EMFILE ||
                 errno == ENFILE || errno == EINTR)
               {
                  break;
               }
             else
               {
                  exit(ERR_ACCEPT_FAILED);
               }
          }

        if (server->socket_count >= server->sockets_max)
          {
             close(sock);
             break;
          }

        if (server->is_tls)
          {
             ssl = SSL_new(server->ctx);
             SSL_set_fd(ssl, sock);

	     while (ret <= 0)
               {
                  ret = SSL_accept(ssl);
                  if (ret <= 0)
                    {
                       int err = SSL_get_error(ssl, ret);
                       if (err == SSL_ERROR_WANT_READ)
                         {
                            continue;
                         }
                       else
                         {
                            SSL_free(ssl);
                            close(sock);
                            break;
                         }
                    }
               }

	     if (ret <= 0) continue;

             if (SSL_accept(ssl) <= 0)
               {
                  SSL_free(ssl);
                  close(sock);
                  continue;
               }
          }

        flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        if (server->is_websocket)
          {
             if (ws_negotiate(sock, ssl))
               {
                  server_client_t *c = _clients_add(server, sock, ssl);
                  if (c)
                    _on_add_cb(server, c);
               }
             else
               {
                  if (ssl)
                    SSL_free(ssl);
                  close(sock);
               }
          }
        else
          {
             server_client_t *c = _clients_add(server, sock, ssl);
             if (c)
               _on_add_cb(server, c);
          }
     }
   while (sock != -1);
}

void
server_run(server_t *server)
{
   sigset_t newmask, oldmask;
   struct pollfd *sockets;
   int i, res, flags;

   if (server->is_tls)
     {
        _tls_init();
        server->ctx = _tls_create(server);
     }

   if (server->protocol == AF_INET)
     {
        server->sock = net_tcp_server_new(server->address, server->port);
     }
   else if (server->protocol == AF_INET6)
     {
        server->sock = net_tcp6_server_new(server->address, server->port);
     }
   else
     {
        server->sock = ipc_unix_server_new(server->address);
     }

   if (server->sock == -1)
     {
        exit(ERR_SOCKET_FAILED);
     }

   flags = fcntl(server->sock, F_GETFL, 0);
   fcntl(server->sock, F_SETFL, flags | O_NONBLOCK);

   server->sockets[0].fd = server->sock;
   server->sockets[0].events = POLLIN;
   server->socket_count = 1;

   sockets = server->sockets;

   sigemptyset(&newmask);
   sigaddset(&newmask, SIGINT);

   while (server->enabled)
     {
#if defined(_DEBUG_)
        printf("\rtotal socks: %5d clients: %5d ", server->socket_count, server->socket_count - 1);
        fflush(stdout);
#endif

        sigprocmask(SIG_BLOCK, &newmask, &oldmask);
        if ((res = poll(sockets, server->sockets_max, 1000 / 4)) < 0)
          {
             exit(ERR_SELECT_FAILED);
          }
        sigprocmask(SIG_UNBLOCK, &oldmask, NULL);

        if (res == 0)
          {
             _clients_timeout_check(server);
             continue;
          }

        for (i = 0; i < server->sockets_max; i++)
          {
             if (sockets[i].revents == 0) continue;

             if (sockets[i].revents != POLLIN)
               {
                  server_client_t *client = _client_by_fd(server->clients, sockets[i].fd);
                  if (client)
                    {
                       _on_del_cb(server, client);
                       _clients_del(server, client);
                    }
                  else
                    {
                       close(sockets[i].fd);
                       sockets[i].fd = -1;
                    }
                  break;
               }

             if (sockets[i].fd == server->sock)
               {
                  _server_accept(server);
               }
             else
               {
                  server_client_t *client = _client_by_fd(server->clients, sockets[i].fd);
                  if (!client)
                    break;

                  if (server->is_websocket)
                    res = ws_client_read(client);
                  else
                    res = _client_read(client);

                  if (res == CLIENT_STATE_ERROR)
                    {
                       _on_err_cb(server, client);
                       _clients_del(server, client);
                    }
                  else if (res == CLIENT_STATE_DISCONNECT)
                    {
                       _on_del_cb(server, client);
                       _clients_del(server, client);
                    }
                  else if (res != CLIENT_STATE_CONTROL_FRAME &&
                           res != CLIENT_STATE_IGNORE)
                    {
                       _on_data_cb(server, client);
                    }
               }
          }
     }
}

server_t *
server_new(void)
{
   server_t *server;
   int i;

   server = calloc(1, sizeof(server_t));
   if (!server)
     return NULL;

   server->poll_array_size = 4096;

   server->sockets = calloc(1, server->poll_array_size * sizeof(struct pollfd));
   if (!server->sockets)
     return NULL;

   for (i = 0; i < server->poll_array_size; i++)
     {
        server->sockets[i].fd = -1;
     }

   server->clients = calloc(1, sizeof(server_client_t *));
   if (!server->clients)
     return NULL;

   server_config_port_set(server, 12345);
   server_config_clients_max_set(server, 128);
   server->enabled = true;
   server->protocol = AF_INET;
   server->client_timeout = CLIENT_TIMEOUT_DEFAULT;

   return server;
}

