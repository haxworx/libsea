#ifndef __URL_H__
#define __URL_H__

#define _DEFAULT_SOURCE 1
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include "hash.h"
#include "net.h"

typedef enum
{
   URL_CALLBACK_DONE,
   URL_CALLBACK_DATA,
} url_callback_t;

#define BUFFER_SIZE   8192

typedef int             (*callback)(void *data);

typedef struct _data_cb_t data_cb_t;
struct _data_cb_t
{
   void *data;
   int   size;
};

typedef struct _url_t url_t;
struct _url_t
{
   url_t            *self;
   int               sock;
   tls_t             *tls;
   char             *host;
   char             *path;
   int               status;
   int               len;
   int               fd;
   bool              connection_ssl;
   bool              print_percent;
   char             *user_agent;
   hash_t           *headers;
   void             *data;
   callback          callback_data;
   callback          callback_done;
};

/* Public API */

url_t *
url_new(const char *url);

int
url_get(url_t *url);

void
url_headers_get(url_t *url);

const char *
url_header_get(url_t *url, const char *name);

void
url_user_agent_set(url_t *url, const char *string);

void
url_finish(url_t *url);

int
url_fd_write_set(url_t *url, int fd);

void
url_callback_set(url_t *url, int type, callback func);

void        fail(char *msg);

#endif
