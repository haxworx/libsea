#include "websocket.h"
#include "hash.h"
#include "strings.h"
#include <openssl/sha.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

static hash_t *
_headers_get(char *buf)
{
   char *key, *start, *end, *p;
   hash_t *headers;
   char uri[1024];

   if (sscanf(buf, "GET %s HTTP/1.1", uri) != 1)
     return NULL;

   headers = hash_new();

   hash_add(headers, "URI", strdup(uri));

   p = buf;
   while (*p)
     {
        start = strchr(p, '\n');
        if (!start)
          break;

        start++;

        end = strchr(start, ':');
        if (!end)
          break;

        *end = '\0';

        key = &start[0];
        start = end + 1;
        if (!start)
          break;

        while (start[0] == ' ')
          start++;

        end = strchr(start, '\r');
        if (!end)
          break;

        *end = '\0';

        hash_add(headers, key, strdup(start));

        p = end + 1;
     }

   return headers;
}

bool
ws_negotiate(int sock, SSL *ssl)
{
   hash_t *headers;
   const char *key, *upgrade, *response;
   char *token;
   ssize_t size;
   char buf[4096];
   unsigned char sha1[SHA_DIGEST_LENGTH * 2 + 1] = {0};

   do {
      if (ssl)
        size = SSL_read(ssl, buf, sizeof(buf) - 1);
      else
        size = read(sock, buf, sizeof(buf) - 1);

      if (size == 0)
        {
           return false;
        }
      else if (size < 0)
        {
           if (errno == EAGAIN || errno == EWOULDBLOCK)
             continue;
           else
             return false;
        }

   } while (size == -1);

   buf[size] = 0x00;

   headers = _headers_get(buf);
   if (!headers)
     return false;

   upgrade = hash_find(headers, "Upgrade");
   key = hash_find(headers, "Sec-WebSocket-Key");

   if (!key || !upgrade || strcasecmp(upgrade, "websocket"))
     {
        hash_free(headers);
        return false;
     }

   snprintf(buf, sizeof(buf), "%s%s", key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
   SHA1((unsigned char *)buf, strlen(buf), sha1);
   token = strings_encode_base64((const char *)sha1);

   response = "HTTP/1.1 101 Switching Protocols\r\n"
              "Upgrade: websocket\r\n"
              "Connection: Upgrade\r\n"
              "Expect: 101\r\n"
              "Sec-WebSocket-Version: 13\r\n"
              "Sec-WebSocket-Accept: %s\r\n\r\n";

   snprintf(buf, sizeof(buf), response, token);

   if (ssl)
     SSL_write(ssl, buf, strlen(buf));
   else
     send(sock, buf, strlen(buf), MSG_NOSIGNAL);

   hash_free(headers);
   free(token);

   return true;
}

void
ws_ping(server_client_t *client, uint64_t payload)
{
   unsigned char frame[10];

   frame[0] = 0x80 | 0x9;
   frame[1] = 8;
   frame[2] = (char) ((payload >> 56) & 0xff);
   frame[3] = (char) ((payload >> 48) & 0xff);
   frame[4] = (char) ((payload >> 40) & 0xff);
   frame[5] = (char) ((payload >> 32) & 0xff);
   frame[6] = (char) ((payload >> 24) & 0xff);
   frame[7] = (char) ((payload >> 16) & 0xff);
   frame[8] = (char) ((payload >> 8) & 0xff);
   frame[9] = (char) (payload & 0xff);

   client->ping_value = payload;

   if (client->ssl)
     SSL_write(client->ssl, frame, sizeof(frame));
   else
     send(client->sock, frame, sizeof(frame), MSG_NOSIGNAL);
}

void
ws_pong_send(server_client_t *client)
{
   char *pong = ws_pong_create(client->received.data, client->received.size);
   if (pong)
     {
        ws_client_write(client, pong, client->received.size);
        free(pong);
     }
}

char *
ws_pong_create(char *frame, size_t len)
{
   char *pong;
   int payload_length;

   if (!frame || len < 2)
     return NULL;

   payload_length = 0x7f & frame[1];
   if (payload_length > 125)
     return NULL;

   frame[0] = (char) 0x80 | 0xa;

   pong = malloc(len);
   memcpy(pong, frame, len);

   return pong;
}

bool
ws_pong_check(char *buf, size_t size, uint64_t expected)
{
   uint64_t value;

   if (size != 8) return false;

   value = (buf[7] & 0xff) | ((buf[6] << 8) & 0xff00) | ((buf[5] << 16) & 0xff0000) | ((buf[4] << 24) & 0xff000000)
          | ((uint64_t)buf[3] << 32 & 0xff00000000) | ((uint64_t)buf[2] << 40 & 0x0000ff0000000000) | ((uint64_t)buf[1] << 48 & 0x00ff000000000000)
          | ((uint64_t)buf[0] << 56 & 0xff00000000000000);

   if (value == expected)
      return true;

   return false;
}

size_t
ws_client_write(server_client_t *client, const char *mesg, size_t length)
{
   unsigned char frame[10];
   int index, index_response, i;

   index = index_response = 0;

   frame[0] = (char) 129;

   if (length <= 125)
     {
        frame[1] = (char) length;
        index = 2;
     }
   else if (length >= 126 && length <= 65535)
     {
        frame[1] = (char) 126;
        frame[2] = (char)((length >> 8) & 0xff);
        frame[3] = (char)(length & 0xff);
        index = 4;
     }
   else
     {
        frame[1] = (char) 127;
        frame[2] = (char)((length >> 56) & 0xff);
        frame[3] = (char)((length >> 48) & 0xff);
        frame[4] = (char)((length >> 40) & 0xff);
        frame[5] = (char)((length >> 32) & 0xff);
        frame[6] = (char)((length >> 24) & 0xff);
        frame[7] = (char)((length >> 16) & 0xff);
        frame[8] = (char)((length >> 8) & 0xff);
        frame[9] = (char)(length & 0xff);

        index = 10;
     }

   char *response = malloc(index + length);

   for (i = 0; i < index; i++)
     {
        response[index_response++] = frame[i];
     }

   for (i = 0; i < (int) length; i++)
     {
        response[index_response++] = mesg[i];
     }

   if (client->ssl)
     SSL_write(client->ssl, response, index + length);
   else
     send(client->sock, response, index + length, MSG_NOSIGNAL);

   free(response);

   return length;
}

static size_t
server_client_read(server_client_t *client, char *buf, size_t len)
{
   if (client->ssl)
     return SSL_read(client->ssl, buf, len);

   return read(client->sock, buf, len);
}

ws_frame_t *
ws_frame_get(server_client_t *client)
{
   ws_frame_t *fr;
   char buf[64];
   ssize_t bytes;

   bytes = server_client_read(client,  buf, 2);
   if (bytes != 2)
     return NULL;

   fr = malloc(sizeof(ws_frame_t));
   if (!fr)
     return NULL;

   fr->type = ws_frame_type_get(buf[0]);
   fr->fin = 0x80 & buf[0];
   fr->masked = 0x80 & buf[1];
   fr->payload = 0x7f & buf[1];

   if (fr->payload == 126)
     {
       bytes = server_client_read(client, buf, 2);
       if (bytes != 2)
         {
            free(fr);
            return NULL;
         }

       fr->total = (buf[1] & 0xff) | ((buf[0] << 8) & 0xff00);
     }
   else if (fr->payload == 127)
     {
        bytes = server_client_read(client, buf, 8);
        if (bytes != 8)
          {
             free(fr);
             return NULL;
          }

        fr->total = (buf[7] & 0xff) | ((buf[6] << 8) & 0xff00) | ((buf[5] << 16) & 0xff0000) | ((buf[4] << 24) & 0xff000000)
          | ((uint64_t)buf[3] << 32 & 0xff00000000) | ((uint64_t)buf[2] << 40 & 0x0000ff0000000000) | ((uint64_t)buf[1] << 48 & 0x00ff000000000000)
          | ((uint64_t)buf[0] << 56 & 0xff00000000000000);
     }
   else
     {
        fr->total = fr->payload;
     }

   bytes = server_client_read(client, buf, 4);
   if (bytes != 4)
     {
        free(fr);
        return NULL;
     }

   snprintf(fr->mask, sizeof(fr->mask), "%c%c%c%c", buf[0], buf[1], buf[2], buf[3]);

   return fr;
}

ws_frame_type_t
ws_frame_type_get(uint8_t byte)
{
   uint8_t opcode = (byte & 0x0f);

   if (opcode == 0)
     return WEBSOCKET_FRAME_CONTINUE;
   else if (opcode == 1)
     return WEBSOCKET_FRAME_TEXT;
   else if (opcode == 2)
     return WEBSOCKET_FRAME_BINARY;
   else if (opcode == 8)
     return WEBSOCKET_FRAME_CLOSE;
   else if (opcode == 9)
     return WEBSOCKET_FRAME_PING;
   else if (opcode == 10)
     return WEBSOCKET_FRAME_PONG;

   return WEBSOCKET_FRAME_UNIMPLEMENTED;
}

char *
ws_unmask(ws_frame_t *fr, char *buf)
{
   char *unmasked, *data;

   if (!buf)
     return NULL;

   if (!fr->masked)
     return buf;

   data = buf;

   unmasked = calloc(1, fr->total * sizeof(char) + 1);
   if (!unmasked)
     return NULL;

   for (int i = 0; i < fr->total; i++)
     {
        unmasked[i] = *data ^ fr->mask[i % 4];

        ++data;
     }

   free(buf);

   unmasked[fr->total] = 0x00;

   return unmasked;
}

int
ws_client_read(server_client_t *client)
{
   ws_frame_t *fr;
   char *incoming;
   char buf[4096];
   ssize_t bytes;
   int ret, total = 0;
   int needed = sizeof(buf);

   fr = ws_frame_get(client);
   if (!fr)
     return CLIENT_STATE_DISCONNECT;

   if (fr->type != WEBSOCKET_FRAME_CONTINUE && client->state != CLIENT_STATE_READ_CONTINUE)
     {
        free(client->received.data);
        client->received.data = NULL;
        client->received.size = 0;
     }

   if (fr->total == 0)
     {
        free(fr);
        return CLIENT_STATE_IGNORE;
     }

   if (fr->type == WEBSOCKET_FRAME_CLOSE)
     {
        free(fr);
        return CLIENT_STATE_DISCONNECT;
     }

   incoming = malloc(fr->total + 1);

   do
     {
        if ((fr->total - total) < needed)
          needed = fr->total - total;

        bytes = server_client_read(client, buf, needed);
        if (bytes == 0)
          {
             free(fr);
             return CLIENT_STATE_DISCONNECT;
          }
        else if (bytes < 0)
          {
             if (client->ssl)
               {
                  free(fr);
                  return CLIENT_STATE_DISCONNECT;
               }

             switch (errno)
               {
                case EAGAIN:
                case EINTR:
                  continue;

                case ECONNRESET:
                  free(fr);
                  return CLIENT_STATE_DISCONNECT;

                default:
                  exit(ERR_READ_FAILED);
               }
          }

       if (bytes > 0)
         {
            memcpy(&incoming[total], buf, bytes);
            total += bytes;
         }

     } while (total < fr->total);

   client->unixtime = time(NULL);

   if (fr->type != WEBSOCKET_FRAME_CONTINUE && client->state != CLIENT_STATE_READ_CONTINUE)
     {
        client->received.data = ws_unmask(fr, incoming);
        client->received.size = fr->total;
     }
   else
     {
        void *tmp = realloc(client->received.data, client->received.size + fr->total);
        client->received.data = tmp;
        char *next = ws_unmask(fr, incoming);
        memcpy(&client->received.data[client->received.size], next, fr->total);
        client->received.size += fr->total;
        free(next);
        client->state = CLIENT_STATE_READ_CONTINUE;
     }

   ret = fr->total;

   switch (fr->type)
     {
      case WEBSOCKET_FRAME_CLOSE:
        ret = CLIENT_STATE_DISCONNECT;
        break;

      case WEBSOCKET_FRAME_BINARY:
        client->received.type = CLIENT_DATA_TYPE_BINARY;
        break;

      case WEBSOCKET_FRAME_TEXT:
        client->received.type = CLIENT_DATA_TYPE_TEXT;
        break;

      case WEBSOCKET_FRAME_PONG:
        if (!ws_pong_check(client->received.data, client->received.size, client->ping_value))
          {
             ret = CLIENT_STATE_DISCONNECT;
             break;
          }
        ret = CLIENT_STATE_CONTROL_FRAME;
        break;

      case WEBSOCKET_FRAME_PING:
        ws_pong_send(client);
        ret = CLIENT_STATE_CONTROL_FRAME;
        break;

      case WEBSOCKET_FRAME_CONTINUE:
        client->state = CLIENT_STATE_READ_CONTINUE;
        client->received.is_continued = true;
        ret = CLIENT_STATE_CONTROL_FRAME;
        break;

      case WEBSOCKET_FRAME_UNIMPLEMENTED:
        fprintf(stderr, "err: unhandled frame!\n");
        break;
     }

   free(fr);

   return ret;
}

