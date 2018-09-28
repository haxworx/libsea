#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "file.h"

char *
ipc_unix_path_create(const char *name, bool global)
{
   char path[108] = { 0 };
   char *tmp;
   const char *home;

   if (global)
     {
        snprintf(path, sizeof(path), "/tmp/%s.0", name);
     }
   else
     {
        home = getenv("HOME");
        if (!home) return NULL;

        tmp = file_path_append(home, ".cache");
        if (!file_exists(tmp))
          file_mkdir(tmp);

        free(tmp);

        snprintf(path, sizeof(path), "%s/%s/%s.0", home, ".cache", name);
     }

   return strdup(path);
}

int
ipc_unix_destroy(int sockfd)
{
   struct sockaddr_un unixname;
   socklen_t size;

   if (sockfd == -1) return -1;

   size = sizeof(struct sockaddr_un);

   if (getsockname(sockfd, (struct sockaddr *) &unixname, &size) == -1)
     return -1;

   close(sockfd);

   if (file_exists(unixname.sun_path))
     file_remove(unixname.sun_path);

   return 0;
}

int
ipc_unix_udp_server_new(const char *path)
{
   struct sockaddr_un unixname;
   int sock;

   if (!path) return -1;

   if (file_exists(path))
     file_remove(path);

   memset(&unixname, 0, sizeof(unixname));
   unixname.sun_family = AF_UNIX;
   strncpy(unixname.sun_path, path, sizeof(unixname.sun_path) - 1);

   if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
     return -1;

   if (bind(sock, (struct sockaddr *)&unixname, sizeof(unixname)) == -1)
     return -1;

   return sock;
}

int
ipc_unix_server_new(const char *path)
{
   struct sockaddr_un unixname;
   int sock;

   if (!path) return -1;

   if (file_exists(path))
     file_remove(path);

   memset(&unixname, 0, sizeof(unixname));
   unixname.sun_family = AF_UNIX;
   strncpy(unixname.sun_path, path, sizeof(unixname.sun_path) - 1);

   if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
     return -1;

   if (bind(sock, (struct sockaddr *)&unixname, sizeof(unixname)) == -1)
     return -1;

   if (listen(sock, 5) == -1)
     return -1;

   return sock;
}

int
ipc_unix_udp_connect(const char *path)
{
   struct sockaddr_un unixname;
   int sock;

   if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
     return -1;

   memset(&unixname, 0, sizeof(unixname));
   unixname.sun_family = AF_UNIX;
   strncpy(unixname.sun_path, path, sizeof(unixname.sun_path) - 1);

   if (connect(sock, (struct sockaddr *)&unixname, sizeof(unixname)) == -1)
     return -1;

   return sock;
}

int
ipc_unix_connect(const char *path)
{
   struct sockaddr_un unixname;
   int sock;

   if (!path) return -1;

   if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
     return -1;

   memset(&unixname, 0, sizeof(unixname));
   unixname.sun_family = AF_UNIX;
   strncpy(unixname.sun_path, path, sizeof(unixname.sun_path) - 1);

   if (connect(sock, (struct sockaddr *)&unixname, sizeof(unixname)) == -1)
     return -1;

   return sock;
}

char *
ipc_unix_address_by_sock(int sock)
{
   struct sockaddr_un unixname;
   socklen_t size = sizeof(struct sockaddr_un);

   if (getsockname(sock, (struct sockaddr *) &unixname, &size) < 0)
     return NULL;

   return strdup(unixname.sun_path);
}


static char *
_ipc_shm_path_create(const char *name)
{
   char path[4096];
#if defined(__linux__)
   return strdup(name);
#else
   snprintf(path, sizeof(path), "/tmp/%s.shm", name);
#endif

   return strdup(path);
}

static char *
_ipc_sem_name_create(const char *name)
{
   char compat_name[4096];

#if defined(__linux__)
   return strdup(name);
#else
   snprintf(compat_name, sizeof(compat_name), "/%s_sem", name);
#endif

   return strdup(compat_name);
}

int
ipc_shm_create(const char *name)
{
   int fd;
   sem_t *sem;
   char *sem_name, *shm_path;

   shm_path = sem_name = NULL;

   shm_path = _ipc_shm_path_create(name);

   shm_unlink(shm_path);

   fd = shm_open(shm_path, O_CREAT | O_RDWR, 0644);
   if (fd == -1)
     {
        goto error;
     }

   sem_name = _ipc_sem_name_create(name);

#if defined(__linux__)
   sem = sem_open(sem_name, O_CREAT | O_RDWR, 0644, 1);
#else
   sem = sem_open(sem_name, O_CREAT, 0644, 1);
#endif
   if (sem == SEM_FAILED)
     {
        close(fd);
	fd = -1;
        goto error;
     }
    sem_close(sem);

error:
   if (sem_name)
     free(sem_name);
   if (shm_path)
     free(shm_path);

   return fd;
}

int
ipc_shm_destroy(const char *name)
{
   char *sem_name, *shm_path;
   int res;

   sem_name = _ipc_sem_name_create(name);
   shm_path = _ipc_shm_path_create(name);

   sem_unlink(sem_name);
   res = shm_unlink(shm_path);

   free(sem_name);
   free(shm_path);

   return res;
}

void *
ipc_shm_read(const char *name, int size)
{
   int fd;
   sem_t *sem;
   void *res, *value;
   char *map, *sem_name, *shm_path;

   sem_name = shm_path = res = NULL;

   shm_path = _ipc_shm_path_create(name);

   fd = shm_open(shm_path, O_RDONLY, 0644);
   if (fd == -1)
     {
        res = NULL;
        goto error;
     }

   sem_name = _ipc_sem_name_create(name);

#if defined(__linux__)
   sem = sem_open(sem_name, O_RDWR);
#else
   sem = sem_open(sem_name, O_CREAT, 0644, 1);
#endif
   if (sem == SEM_FAILED)
     {
        res = NULL;
        goto error;
     }

   map = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
   if (map == MAP_FAILED)
     {
        res = NULL;
        goto error;
     }

   value = malloc(size);

   if (!sem_wait(sem))
     {
        memcpy(value, map, size);
        res = value;
        sem_post(sem);
     }

   if (munmap(map, size) == -1)
     res = NULL;

   close(fd);
   sem_close(sem);
error:
   if (sem_name)
     free(sem_name);
   if (shm_path)
     free(shm_path);

   return res;
}

int
ipc_shm_write(const char *name, void *data, int size)
{
   char *map, *ptr, *sem_name, *shm_path;
   sem_t *sem;
   int fd, res = 0;

   sem_name = shm_path = NULL;

   shm_path = _ipc_shm_path_create(name);

   fd = shm_open(shm_path, O_RDWR, 0644);
   if (fd == -1)
     {
        res = -1;
        goto error;
     }

   ftruncate(fd, size);

   sem_name = _ipc_sem_name_create(name);
#if defined(__linux__)
   sem = sem_open(sem_name, O_RDWR);
#else
   sem = sem_open(sem_name, O_CREAT, 0644, 1);
#endif
   if (sem == SEM_FAILED)
     {
        res = -1;
        goto error;
     }

   map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (map == MAP_FAILED)
     {
        res = -1;
        goto error;
     }

   ptr = map;

   if (!sem_wait(sem))
     {
        memcpy(ptr, data, size);
        sem_post(sem);
     }

   if (munmap(map, size) == -1)
     res = -1;

   close(fd);
   sem_close(sem);
error:
   if (sem_name)
     free(sem_name);
   if (shm_path)
     free(shm_path);

   return res;
}

static bool
_socket_send(int sock, int fd)
{
   struct cmsghdr *cmsg;
   struct msghdr msg; 
   char buf[CMSG_SPACE(sizeof(int))];

   memset(buf, 0, sizeof(buf));
   memset(&msg, 0, sizeof(struct msghdr));

   struct iovec io;
   io.iov_base  = "";
   io.iov_len = 1;

   msg.msg_iov = &io;
   msg.msg_iovlen = 1;
   msg.msg_control = buf;
   msg.msg_controllen = sizeof(buf);

   cmsg = CMSG_FIRSTHDR(&msg);
   cmsg->cmsg_level = SOL_SOCKET;
   cmsg->cmsg_type = SCM_RIGHTS;
   cmsg->cmsg_len = CMSG_LEN(sizeof(int));

   memmove(CMSG_DATA(cmsg), &fd, sizeof(int));
   msg.msg_controllen = cmsg->cmsg_len;

   if (sendmsg(sock, &msg, 0) == -1)
     {
        return false;
     }

   return true;
}

static int
_fd_receive(int sock)
{
   struct cmsghdr *cmsg;
   int fd;
   struct msghdr msg; 
   char m_buffer[1], c_buffer[512];
   struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };

   memset(&msg, 0, sizeof(struct msghdr));

   msg.msg_iov = &io;
   msg.msg_iovlen = 1;
   msg.msg_control = c_buffer;
   msg.msg_controllen = sizeof(c_buffer);

   if (recvmsg(sock, &msg, 0) < 0) return -1;

   cmsg = CMSG_FIRSTHDR(&msg);

   memmove(&fd, CMSG_DATA(cmsg), sizeof(int));

   return fd;
}

void
ipc_sock_transfer(int sock, void (*new_process)(int))
{
   int socks[2];

   if (socketpair(AF_UNIX, SOCK_DGRAM, 0, socks) == -1)
     return;

   pid_t pid = fork();
   if (pid < 0)
     exit(EXIT_FAILURE);
   else if (pid > 0)
     {
        close(socks[1]);
        _socket_send(socks[0], sock);
        close(sock);
     }
   else
     {
        close(socks[0]);
        int fd = _fd_receive(socks[1]);
        if (fd != -1)
          {
             if (new_process)
               new_process(fd);
          }
        exit(EXIT_SUCCESS);
     }
}

