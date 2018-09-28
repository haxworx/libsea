#ifndef __IPC_H__
#define __IPC_H__

/**
 * @file
 * @brief Inter Process Communcation.
 */

/**
 * @brief IPC methods.
 * @defgroup IPC
 *
 * @{
 *
 * Inter Process Communcation.
 *
 */

#include <stdbool.h>

/**
 * Create a new UNIX datagram socket server.
 *
 * @param path The location of the socket within the filesystem.
 *
 * @return The socket's file descriptor.
 */
int
ipc_unix_udp_server_new(const char *path);

/**
 * Create a new UNIX domain socket server.
 *
 * @param path The location of the socket on the filesystem.
 *
 * @return The socket's file descriptor.
 */
int
ipc_unix_server_new(const char *path);

/**
 * Connect to a UNIX domain socket server at path.
 *
 * @param path The location of the socket on the filesystem.
 *
 * @return The file descriptor connected to the socket.
 */
int
ipc_unix_connect(const char *path);

/**
 * Connect to a UNIX datagram socket server.
 *
 * @param path The location of the socket on the filesystem.
 *
 * @return The file descriptor connected to the socket.
 */
int
ipc_unix_udp_connect(const char *path);

/**
 * Close, destroy and unlink UNIX domain socket server instance.
 *
 * @param sockfd The server's socket.
 *
 * @return Returns 0 on success and non-zero on failure.
 */
int
ipc_unix_destroy(int sockfd);

/**
 * Return the address (as a string) of a connected UNIX domain socket.
 *
 * @param sock The socket to obtain the address from.
 *
 * @return A newly allocated string containing the address.
 */
char *
ipc_unix_address_by_sock(int sock);

/**
 * Helper to create a valid path for a UNIX domain socket on a filesystem.
 *
 * @param name The unique name of the resource.
 * @param global Whether the path shold be system-wide (true) or local to a user (false).
 *
 * @return A newly allocated string with the suggested path.
 */
char *
ipc_unix_path_create(const char *name, bool global);

/**
 * Create a new instance for sharing memory between processes identified by a common name.
 *
 * @param name The unique name of the shared memory instance.
 *
 * @return Returns 0 on success and non-zero on failure.
 */
int
ipc_shm_create(const char *name);

/**
 * Write data to the shared memory location identified by its name.
 *
 * @param name The unique name of the shared memory instance.
 * @param data The data to write to shared memory.
 * @param size The length of the data in bytes to be written.
 *
 * @return Returns 0 on success and non-zero on failure.
 */
int
ipc_shm_write(const char *name, void *data, size_t size);

/**
 * Read data from the shared memory location identified by its name.
 *
 * @param name The unique name of the shared memory instance.
 * @param size The size in bytes to read from the shared memory instance.
 */
void *
ipc_shm_read(const char *name, int size);

/**
 * Remove all instances of the shared memory on the system with given name.
 *
 * @param name The unique name of the shared memory instance to be removed.
 *
 * @return Returns 0 on success and non-zero on failure.
 */
int
ipc_shm_destroy(const char *name);


/**
 * Transfer a socket to another process.
 *
 * @param sock The socket to transfer (will be closed).
 * @param new_process The function pointer within a new process to be executed with the transferred socket.
 */
void
ipc_sock_transfer(int sock, void (*new_process)(int));

/**
 * @}
 */

#endif
