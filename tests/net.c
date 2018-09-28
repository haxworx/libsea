#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *addresses[] = {
   "haxlab.org:8192",
   "nipl.net:22",
   "enlighenment.org:3128",
   "thisisntreal.shuf:9999",
   "linux.com:1",
   "::1:22",
   "127.0.0.1:22",
   "127.0.0.1:6789",
};

static int
try(const char *address)
{
   char *remote;
   char buf[8192] = { 0 };
   ssize_t bytes;
   int sock;

   printf("testing: %s\n", address);

   sock = net_tcp_connect(address);
   if (sock == -1) return 1;

   bytes = read(sock, buf, sizeof(buf));
   if (bytes < 0) return 2;

   buf[bytes] = 0x00;

   remote = net_tcp_address_by_sock(sock);
   if (remote)
     {
        printf("remote: %s sent: %s\n", remote ,buf);
        free(remote);
     }

   close(sock);

   return 0;
}

int
main(void)
{
   int sock = net_tcp_server_new("127.0.0.1", 5432);

   printf("sock is %d\n", sock);
   close(sock);

   sock = net_tcp_server_new("99.99.99.99", 1234);
   printf("sock is %d\n", sock);
   close(sock);

   printf("timeout %d\n", net_tcp_connect_timeout_get());
   net_tcp_connect_timeout_set(3);
   printf("timeout %d\n", net_tcp_connect_timeout_get());

   tls_t *tls = net_tls_connect("google.com:443");
   if (tls)
     {
        net_tls_close(tls);
        net_tls_free(tls);
     }

   for (int i = 0; i < sizeof(addresses) / sizeof(char *); i++)
      try(addresses[i]);

   return 0;
}
