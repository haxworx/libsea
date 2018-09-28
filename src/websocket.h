#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <stdbool.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include "server.h"

typedef enum
{
   WEBSOCKET_FRAME_CONTINUE,
   WEBSOCKET_FRAME_TEXT,
   WEBSOCKET_FRAME_BINARY,
   WEBSOCKET_FRAME_CLOSE,
   WEBSOCKET_FRAME_PING,
   WEBSOCKET_FRAME_PONG,
   WEBSOCKET_FRAME_UNIMPLEMENTED,
} ws_frame_type_t;

typedef struct ws_frame_t ws_frame_t;
struct ws_frame_t
{
   ws_frame_type_t type;
   bool            fin;
   int             opcode;

   bool            masked;
   char            mask[4 + 1];

   int             payload;
   int32_t         total;
};

ws_frame_type_t
ws_frame_type_get(uint8_t byte);

ws_frame_t *
ws_frame_get(server_client_t *client);

bool
ws_negotiate(int sock, SSL *ssl);

size_t
ws_client_write(server_client_t *client, const char *mesg, size_t length);

int
ws_client_read(server_client_t *client);

char *
ws_unmask(ws_frame_t *frame, char *buf);

void
ws_ping(server_client_t *client, uint64_t payload);

bool
ws_pong_check(char *buf, size_t size, uint64_t expected);

char *
ws_pong_create(char *frame, size_t len);

void
ws_pong_send(server_client_t *client);


#endif
