#include "server.h"
#include <string.h>

static int
_on_add_cb(server_event_t *event, void *data)
{
   server_t *server;
   char *address;
   const char *welcome = "bonjour monde!\r\n";

   server = event->server;

   server_client_write(event->client, welcome, strlen(welcome));

   address = server_client_address_get(event->client);
   if (address)
     {
        printf("added client #%d origin: %s\n", server->socket_count - 1, address);
        free(address);
     }

   return 0;
}

static int
_on_data_cb(server_event_t *event, void *data)
{
   server_t *server;
   char *string;
   int len;
   server_data_t *received;

   server = event->server;
   received = event->received;

   len = received->size;

   if (!server_received_is_text(received))
     return 0;

   string = malloc(len + 1);
   memcpy(string, received->data, received->size);
   string[len] = 0x00;

   printf("received: %s length: %d sock: %d\n\n",
          string, received->size, event->client->sock);

   if (!strncmp(string, "cheese", 6))
     {
        const char *msg = "That's offensive!\r\n";
        server_client_write(event->client, msg, strlen(msg));
        server_client_del(server, event->client);
     }

   free(string);

   return 0;
}

static int
_on_del_cb(server_event_t *event, void *data)
{
   printf("disconnected: %d\n", event->client->sock);

   return 0;
}

static void
usage(void)
{
   printf("Usage: server [ADDRESS]\n"
          "   Where ADDRESS can be one of\n"
          "   IP:PORT or IPv6:PORT or file://PATH\n"
          "   e.g. 10.1.1.1:12345         (listen on 10.1.1.1 port 12345)\n"
          "        ::1:12345              (listen on ::1 port 12345\n"
          "        *:4444                 (listen on all addresses port 4444\n"
          "        file:///tmp/socket.0   (listen on UNIX local socket at path)\n");

   exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
   server_t *server;
   const char *address;

   if (argc == 2)
     {
        address = argv[1];
     }
   else
     {
        usage();
     }

   server = server_new();
   if (!server)
     exit(EXIT_FAILURE);

   if (!server_config_address_port_set(server, address))
     {
        fprintf(stderr, "Invalid listen address and port\n");
        exit(EXIT_FAILURE);
     }

   server_config_clients_max_set(server, 512);

   server_config_client_timeout_set(server, 60);

   printf("PID %d listening at %s, max clients %d\n",
        getpid(), server->local_address, server->sockets_max - 1);

   server_event_callback_set(server, SERVER_EVENT_CALLBACK_ADD, _on_add_cb, NULL);
   server_event_callback_set(server, SERVER_EVENT_CALLBACK_DEL, _on_del_cb, NULL);
   server_event_callback_set(server, SERVER_EVENT_CALLBACK_ERR, _on_del_cb, NULL);
   server_event_callback_set(server, SERVER_EVENT_CALLBACK_DATA, _on_data_cb, NULL);

 //  server_tls_enabled_set(server, true);
 //  server_tls_keys_set(server, "server.crt", "server.key");
 //  server_websocket_enabled_set(server, true);

   server_run(server);

   server_free(server);

   return EXIT_SUCCESS;
}

