#include "errors.h"
#include "url.h"
#include "net.h"
#include <ctype.h>

static char *
_host_from_url(const char *host)
{
   char *addr, *end, *tmp = strdup(host);

   char *str = strstr(tmp, "http://");
   if (str)
     {
        char *start = tmp;
        tmp += strlen("http://");
        end = strchr(tmp, '/');
        if (end)
          {
             *end = '\0';
          }
        addr = strdup(tmp);
        free(start);
        return addr;
     }

   str = strstr(tmp, "https://");
   if (str)
     {
        char *start = tmp;
        tmp += strlen("https://");
        end = strchr(tmp, '/');
        if (end)
          {
             *end = '\0';
          }
        addr = strdup(tmp);
        free(start);
        return addr;
     }

   return tmp;
}

static char *
_path_from_url(const char *path)
{
   char *tmp, *addr = strdup(path);
   char *p = addr;

   char *str = strstr(addr, "http://");
   if (str)
     {
        str += 7;
        char *p = strchr(str, '/');
        if (p)
          {
             tmp = strdup(p);
             free(addr);
             return tmp;
          }
        else
          {
             free(addr);
             return strdup("/");
          }
     }

   str = strstr(addr, "https://");
   if (str)
     {
        str += 8;
        char *p = strchr(str, '/');
        if (p)
          {
             tmp = strdup(p);
             free(addr);
             return tmp;
          }
        else
          {
             free(addr);
             return strdup("/");
          }
     }

   return p;
}

static tls_t *
_connect_tls(const char *hostname, int port)
{
   tls_t *tls;
   char address[4096];

   snprintf(address, sizeof(address), "%s:%d", hostname, port);

   tls = net_tls_connect(address);

   return tls;
}

static int
_connect_tcp(const char *hostname, int port)
{
   char address[4096];
   int sock;

   snprintf(address, sizeof(address), "%s:%d", hostname, port);

   sock = net_tcp_connect(address);

   return sock;
}

static ssize_t
Write(url_t *url, char *bytes, size_t len)
{
   if (url->connection_ssl)
     {
        return BIO_write(url->tls->bio, bytes, len);
     }

   return write(url->sock, bytes, len);
}

static ssize_t
Read(url_t *url, char *buf, size_t len)
{
   if (url->connection_ssl)
     {
        return BIO_read(url->tls->bio, buf, len);
     }

   return read(url->sock, buf, len);
}

const char *
url_header_get(url_t *url, const char *name)
{
   return hash_find(url->headers, name);
}

static int
_chunk_size(url_t *url)
{
   char buf[64], byte[1];
   unsigned int chunk;
   int i = 0;

   while (1)
     {
        Read(url, byte, 1);
        buf[i] = byte[0];

        if (i && buf[i - 1] == '\r' && buf[i] == '\n')
          {
             buf[i - 1] = 0x00;
             if (strlen(buf) == 0)
               {
                  i = 0;
                  continue;
               }
             break;
          }
        ++i;
     }
   chunk = strtol(buf, NULL, 16);

   return chunk;
}

static void
_http_content_chunked_get(url_t *url)
{
   char *buf;
   int chunk, count;
   unsigned int bytes = 0, total = 0;

   if (!url->callback_data && url->fd == -1)
     {
        url->len = 0;
        url->data = malloc(sizeof(char *));
     }

   while (1)
     {
        chunk = _chunk_size(url);
        if (chunk == 0)
          break;

        buf = malloc(sizeof(char) * chunk);

        count = 0;
        do
          {
             bytes = Read(url, &buf[count], chunk - count);
             if (bytes <= 0) break;
             count += bytes;
          }
        while (count != chunk);

        bytes = count;
        total += bytes;

        if (url->callback_data)
          {
             data_cb_t *received = malloc(sizeof(data_cb_t));
             received->data = buf;
             received->size = bytes;
             url->callback_data(received);
             free(received);
          }
        else
          {
             if (url->fd >= 0)
               {
                  write(url->fd, buf, bytes);
               }
             else if (bytes > 0)
               {
                  void *tmp = realloc(url->data, 1 + url->len + bytes);
                  url->data = tmp;
                  unsigned char *pos = (unsigned char *) url->data + url->len;
                  url->len += bytes;

                  memcpy(pos, buf, bytes);
               }
          }

        free(buf);
    }

   if (url->callback_done)
     {
        url->callback_done(NULL);
     }
}

static void
_http_content_get(url_t *url)
{
   const char *encoding, *content_length;
   char buf[BUFFER_SIZE];
   ssize_t count;
   unsigned int bytes, length = 0, total = 0;

   encoding = url_header_get(url, "Transfer-Encoding");
   if (encoding && !strcasecmp(encoding, "chunked"))
     {
        _http_content_chunked_get(url);
        return;
     }

   content_length = url_header_get(url, "Content-Length");
   if (!content_length)
     return;

   length = url->len = atoi(content_length);
   if (!length)
     return;

   if (!url->callback_data && url->fd == -1)
     {
        url->data = calloc(1, length);
     }

   do
     {
        bytes = 0;

        while (bytes < BUFFER_SIZE)
          {
             if (length && (total + bytes) == length) break;

             count = Read(url, &buf[bytes], BUFFER_SIZE - bytes);
             if (count <= 0)
               {
                  break;
               }
             bytes += count;
          }

        if (url->callback_data)
          {
             data_cb_t *received = malloc(sizeof(data_cb_t));
             received->data = buf;
             received->size = bytes;
             url->callback_data(received);
             free(received);
          }
        else
          {
             if (url->fd >= 0)
               {
                  write(url->fd, buf, bytes);
               }
             else
               {
                  unsigned char *pos = (unsigned char *)url->data + total;
                  memcpy(pos, buf, bytes);
               }
          }

        total += bytes;

        if (count <= 0) break;

        if (length && total >= length) break;

     }
   while (1);

   if (url->callback_done)
     {
        url->callback_done(NULL);
     }
}

static void
_http_connect(url_t *url)
{
   if (url->connection_ssl)
     {
        url->tls = _connect_tls(url->host, 443);
        if (!url->tls)
          errors_fail("unable to connect");
     }
   else
     {
        url->sock = _connect_tcp(url->host, 80);
        if (url->sock == -1)
          errors_fail("unable to connect");
     }

   if (url->tls || url->sock)
     {
        char query[4096];

        snprintf(query, sizeof(query), "GET /%s HTTP/1.1\r\n"
                                       "User-Agent: %s\r\n"
                                       "Accept: */*\r\n"
                                       "Host: %s\r\n\r\n", url->path, url->user_agent, url->host);

        Write(url, query, strlen(query));
     }
}

static bool
_http_headers_get(url_t *url)
{
   char *buf, *key, *value;
   char *p, *start, *end;
   int len = 0, bytes = 0, buf_size;
   bool ret = false;

   _http_connect(url);

   url->headers = hash_new();

   buf_size = 128;
   buf = malloc(sizeof(char) * buf_size);

   while (1)
     {
        if (len >= buf_size)
          {
             buf_size <<= 1;
             void *tmp = realloc(buf, buf_size * sizeof(char));
             buf = tmp;
          }

        bytes = Read(url, &buf[len], 1);
        if (bytes <= 0)
          {
             goto done;
          }

        if (len >= 4 && buf[len - 3] == '\r' && buf[len - 2] == '\n' &&
            buf[len - 1] == '\r' && buf[len] == '\n')
          {
             ret = true;
             break;
          }
        len += bytes;
     }

   buf[len] = '\0';

   int count = sscanf(buf, "\nHTTP/1.1 %d", &url->status);
   if (!count) goto done;

   p = buf;
   while (*p)
     {
        start = strchr(p, '\n');
        if (!start) break;

        start++;
        end = strchr(start, ':');

        if (!end) break;
        *end = '\0';

        key = start;
        start = end + 1;
        value = start;

        while (isspace(value[0]))
          {
             value++;
          }

        end = strchr(start, '\r');
        if (!end) break;
        *end = '\0';

        hash_add(url->headers, key, strdup(value));
        p = end + 1;
     }
done:

   free(buf);

   return ret;
}

void
url_user_agent_set(url_t *url, const char *string)
{
   if (url->user_agent) free(url->user_agent);
   url->user_agent = strdup(string);
}

url_t *
url_new(const char *addr)
{
   url_t *url = calloc(1, sizeof(url_t));

   url->fd = -1;

   if (!url->user_agent)
     {
        url->user_agent = strdup
            ("Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) "
             "AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 "
             "Mobile Safari/535.19");
     }

   if (!strncmp(addr, "https://", 8))
     {
        url->connection_ssl = true;
     }

   url->host = _host_from_url(addr);
   url->path = _path_from_url(addr);

   url->self = url;

   return url;
}

int
url_get(url_t *url)
{
   if (url->headers == NULL)
     {
        if (!_http_headers_get(url))
          errors_fail("unable to obtain HTTP headers.");
     }

   _http_content_get(url);

   return url->status;
}

void
url_headers_get(url_t *url)
{
   if (!_http_headers_get(url))
     errors_fail("unable to obtain HTTP headers.");
}

void
url_finish(url_t *url)
{
   if (url->connection_ssl)
     {
        net_tls_free(url->tls);
     }
   else if (url->sock >= 0)
     {
        close(url->sock);
     }

   if (url->fd >= 0)
     {
        close(url->fd);
     }

   if (url->host) free(url->host);
   if (url->path) free(url->path);

   hash_free(url->headers);

   if (url->user_agent)
     free(url->user_agent);

   if (url->data)
     free(url->data);

   free(url);
}

int
url_fd_write_set(url_t *url, int fd)
{
   if (fd >= 0)
     {
        url->fd = fd;
        return fd;
     }
   return -1;
}

void
url_callback_set(url_t *url, int type, callback func)
{
   switch (type)
     {
      case URL_CALLBACK_DATA:
        url->callback_data = func;
        break;

      case URL_CALLBACK_DONE:
        url->callback_done = func;
     }
}

