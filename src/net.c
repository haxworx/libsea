#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <netdb.h>
#include "errors.h"
#include "net.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L
# define TLS_client_method TLSv1_2_client_method
#endif

static int _tcp_connect_timeout = 5;
static int _tls_connect_timeout = 5;

void
net_tcp_connect_timeout_set(int timeout)
{
   _tcp_connect_timeout = timeout;
}

int
net_tcp_connect_timeout_get(void)
{
   return _tcp_connect_timeout;
}

void
net_tls_connect_timeout_set(int timeout)
{
   _tls_connect_timeout = timeout;
}

int
net_tls_connect_timeout_get(void)
{
   return _tls_connect_timeout;
}

int
net_tcp_connect(const char *address)
{
   int flags, sock;
   struct addrinfo hints, *res, *res0;
   char *port, *hostname;
   struct timeval tm;
   fd_set fds;

   flags = sock = -1;
   res0 = NULL;
   hostname = strdup(address);

   port = strrchr(hostname, ':');
   if (!port) goto cleanup;

   *port++ = '\0';

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(hostname, port, &hints, &res0) != 0)
     goto cleanup;

   for (res = res0; res; res = res->ai_next)
     {
        if (res->ai_family != AF_INET && res->ai_family != AF_INET6)
          continue;

        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock == -1)
          goto cleanup;

        flags = fcntl(sock, F_GETFL, 0);
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        tm.tv_sec = net_tcp_connect_timeout_get();
        tm.tv_usec = 0;

        int status = connect(sock, res->ai_addr, res->ai_addrlen);
        if (status == -1 && errno != EINPROGRESS)
          break;

        status = select(sock + 1, NULL, &fds, NULL, &tm);
        if (status == 0)
          {
             close(sock);
             sock = -1;
             goto cleanup;
          }

        break;
    }
    if (sock != -1)
      fcntl(sock, F_SETFL, flags);

cleanup:
    free(hostname);
    if (res0)
      freeaddrinfo(res0);

    return sock;
}

int
net_tcp6_server_new(const char *address, int port)
{
   struct sockaddr_in6 servername;
   int reuseaddr = 1;
   int sock;

   if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
     return -1;

   memset(&servername, 0, sizeof(servername));
   servername.sin6_family = AF_INET6;
   servername.sin6_port = htons(port);

   if (address)
     {
        inet_pton(AF_INET6, address, &servername.sin6_addr);
     }
   else
     {
        servername.sin6_addr = in6addr_any;
     }

   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0)
     return -1;

   if (bind(sock, (struct sockaddr *) &servername, sizeof(servername)) < 0)
     return -1;

   if (listen(sock, 5) < 0)
     return -1;

   return sock;
}

int
net_tcp_server_new(const char *address, int port)
{
   struct sockaddr_in servername;
   int sock, reuseaddr =1;

   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
     return -1;

   memset(&servername, 0, sizeof(servername));
   servername.sin_family = AF_INET;
   servername.sin_port = htons(port);

   if (address)
     {
        servername.sin_addr.s_addr = inet_addr(address);
     }
   else
     {
        servername.sin_addr.s_addr = INADDR_ANY;
     }

   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0)
     return -1;

   if (bind(sock, (struct sockaddr *) &servername, sizeof(servername)) < 0)
     return -1;

   if (listen(sock, 5) < 0)
     return -1;

   return sock;
}


char *
net_tcp6_address_by_sock(int sock)
{
   char *address;
   struct sockaddr_in6 client_addr;
   char ip[INET6_ADDRSTRLEN];
   int port, len;
   socklen_t size;

   size = sizeof(struct sockaddr_in6);

   if (getpeername(sock, (struct sockaddr *)&client_addr, &size) < 0)
     return NULL;

   if (!inet_ntop(AF_INET6, &client_addr.sin6_addr, ip, INET6_ADDRSTRLEN))
     {
        return NULL;
     }

   port = ntohs(client_addr.sin6_port);

   len = INET6_ADDRSTRLEN + 8;

   address = malloc(len);

   snprintf(address, len, "%s:%d", ip, port);

   return address;
}

char *
net_tcp_address_by_sock(int sock)
{
    char *address;
    struct sockaddr_in client_addr;
    char ip[INET_ADDRSTRLEN];
    int port, len;
    socklen_t size;

    size = sizeof(struct sockaddr_in);

    if (getpeername(sock, (struct sockaddr *)&client_addr, &size) < 0)
      return NULL;

    if (client_addr.sin_family == AF_INET6)
      return net_tcp6_address_by_sock(sock);

    strncpy(ip, inet_ntoa(client_addr.sin_addr), INET_ADDRSTRLEN);

    port = ntohs(client_addr.sin_port);

    len = INET_ADDRSTRLEN + 8;

    address = malloc(len);

    snprintf(address, len, "%s:%d", ip, port);

    return address;
}

tls_t *
net_tls_connect(const char *address)
{
   tls_t *tls;
   BIO *bio;
   SSL_CTX *ctx;
   SSL *ssl = NULL;

   SSL_load_error_strings();
   ERR_load_BIO_strings();
   OpenSSL_add_all_algorithms();

   SSL_library_init();

   tls = calloc(1, sizeof(tls_t));
   if (!tls)
     {
        WARN("calloc");
        return NULL;
     }

   ctx = SSL_CTX_new(TLS_client_method());

   tls->ctx = ctx;

   SSL_CTX_set_timeout(ctx, net_tls_connect_timeout_get());
   SSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs", NULL);

   bio = BIO_new_ssl_connect(ctx);
   if (!bio)
     {
        WARN("BIO_new_ssl_connect");
        net_tls_free(tls);
        return NULL;
     }

   tls->bio = bio;

   BIO_get_ssl(bio, &ssl);
   SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
   BIO_set_conn_hostname(bio, address);

   if (BIO_do_connect(bio) <= 0)
     {
        WARN("BIO_do_connect");
        net_tls_free(tls);
        return NULL;
     }

   return tls;
}

void
net_tls_free(tls_t *tls)
{
   net_tls_close(tls);

   free(tls);
   tls = NULL;
}

void
net_tls_close(tls_t *tls)
{
   if (tls->bio)
     {
        BIO_free_all(tls->bio);
        tls->bio = NULL;
     }

   if (tls->ctx)
     {
        SSL_CTX_free(tls->ctx);
        tls->ctx = NULL;
     }
}
