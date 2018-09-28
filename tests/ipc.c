#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc.h"
#include "net.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

typedef struct employee_t
{
   char name[128];
   int value;
} employee_t;

void test_process(int fd)
{
   close(0); close(1); close(2);
   dup(fd); dup(fd); dup(fd);
   printf("Launching a shell! :)\n");
   system("/bin/sh");
}

int
main(void)
{
   pid_t pid;

   char *path = ipc_unix_path_create("cheese", false);

   int res = ipc_shm_create("sausages");
   if (res == -1)
     exit (1 << 0);

   pid = fork();
   if (pid < 0)
     exit(1 << 1);

   if (pid == 0)
     {
        employee_t emp;
        snprintf(emp.name, sizeof(emp.name), "Duke of Wellington");
        emp.value = 99;
        res = ipc_shm_write("sausages", &emp, sizeof(employee_t));
        printf("res %d pid %d done ipc_shm_write()\n", res, getpid());
        int client_sock = ipc_unix_udp_connect(path);
        if (client_sock == -1)
          exit(1 << 1);
        for (int i = 0; i < 5; i++)
          {
             char tmp[1024];
             snprintf(tmp, sizeof(tmp), "hello world %d\n", i);
             write(client_sock, tmp, strlen(tmp));
             sleep(1);
          }
        write(client_sock, "", 0);
        free(path);
        exit(EXIT_SUCCESS);
     }
   else
     {
        char buf[1024] = { 0 };
        ssize_t bytes;
        int sock = ipc_unix_udp_server_new(path);
        sleep(2);
        employee_t *t = ipc_shm_read("sausages", sizeof(employee_t));
        if (t)
          {
             printf("pid %d done ipc_shm_read()\n", getpid());
             printf("name: %s and value %d\n", t->name, t->value);
             free(t);
          }
        do
          {
             bytes = read(sock, buf, sizeof(buf));
             if (bytes < 0) exit(1 << 2);
             else if (bytes > 0)
               {
                  printf("is %s\n", buf);
               }
          }
        while (bytes != 0);
        ipc_unix_destroy(sock);
     }

   free(path);

   ipc_shm_destroy("sausages");

   /* Start of ipc_sock_transfer() example */

   printf("Connect on 127.0.0.1:12345 (will timeout)\n");
   int sock = net_tcp_server_new("127.0.0.1", 12345);
   if (sock == -1)
     exit(1 << 3);

   int flags = fcntl(sock, F_GETFL, 0);
   fcntl(sock, F_SETFL, flags | O_NONBLOCK);
   fd_set fds;
   struct timeval tm = { 5, 0 };

   FD_ZERO(&fds);
   FD_SET(sock, &fds);

   res = select(sock + 1, &fds, NULL, NULL, &tm);
   if (res == 0)
     {
        printf("Timeout on test, please connect 127.0.0.1:12345\n");
        exit(EXIT_SUCCESS);
     }

   struct sockaddr_in clientname;
   socklen_t size = sizeof(clientname);

   int incoming = accept(sock, (struct sockaddr *)&clientname, &size);
   if (incoming < 0)
     exit(1 << 4);

   const char *message = "You are about to enter the twilight zone!!!\r\n";
   write(incoming, message, strlen(message));
   sleep(2);

   ipc_sock_transfer(incoming, test_process);

   close(sock);

   return EXIT_SUCCESS;
}

