#include "errors.h"
#include "url.h"
#include <sys/stat.h>

int fd = -1;
int is_chardev = 0;
SHA256_CTX ctx;

void
usage(void)
{
   printf("./urltest <url> <file>\n");
   exit(EXIT_FAILURE);
}

int
data_done_cb(void *data)
{
   return close(fd);
}

int
data_received_cb(void *data)
{
   char buffer[BUFFER_SIZE];
   data_cb_t *received = data;
   if (!received) return 0;

   char *pos = received->data;

   if (is_chardev && (received->size < BUFFER_SIZE))
     {
        memcpy(buffer, received->data, received->size);
        memset(&buffer[received->size], 0, BUFFER_SIZE - received->size);
        received->size = sizeof(buffer);
        pos = &buffer[0];
     }

   SHA256_Update(&ctx, received->data, received->size);

   return write(fd, pos, received->size);
}

int
main(int argc, char **argv)
{
   struct stat fstats;
   int status, i = 0, j = 0;

   if (argc != 3) usage();

   url_t *req = url_new(argv[1]);

   SHA256_Init(&ctx);

   fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
   if (fd == -1)
     {
        errors_fail("open()");
     }

   stat(argv[2], &fstats);
   if (S_ISCHR(fstats.st_mode)) is_chardev = 1;

   url_callback_set(req, URL_CALLBACK_DATA, data_received_cb);
   url_callback_set(req, URL_CALLBACK_DONE, data_done_cb);

   /* override default user-agent string */
   url_user_agent_set(req,
                       "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36");

   printf("User-Agent: %s\n", req->user_agent);

   url_headers_get(req);

   char **keys = hash_keys_get(req->headers);

   for (int i = 0; keys[i]; i++)
     {
        printf("%s => %s\n", keys[i], hash_find(req->headers, keys[i]));
     }

   hash_keys_free(keys);

   status = url_get(req);
   switch (status)
     {
      case 404:
        errors_fail("404 not found!");
        break;
     }

   if (status != 200)
     {
        errors_fail("status is not 200!");
     }

   unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
   SHA256_Final(result, &ctx);

   char sha256sum[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
   for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
     {
        snprintf(&sha256sum[j], sizeof(sha256sum), "%02x", (unsigned int)result[i]);
        j += 2;
     }

   url_finish(req);

   printf("SHA256 = %s\n", sha256sum);
   printf("done!\n");

   return EXIT_SUCCESS;
}

