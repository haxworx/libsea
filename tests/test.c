/* Just a test for the library code */

#include "btree.h"
#include "buf.h"
#include "list.h"
#include "hash.h"
#include "system.h"
#include "file.h"
#include "exe.h"
#include "strings.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <openssl/sha.h>

typedef struct employee_t
{
   unsigned int id;
   const char *name;
} employee_t;

employee_t employees[] =  {
   { .id = 1, .name = "Alastair"},
   { .id = 2, .name = "Ed" },
   { .id = 3, .name = "Jimmy" },
};

static void
test_tree(void)
{
   employee_t *found;
   tree_t *tree = tree_new();

   for (int i = 0; i < 3; i++)
     {
         employee_t *emp = malloc(sizeof(employee_t));
         memcpy(emp, &employees[i], sizeof(employee_t));
         tree = tree_add(tree, emp->id, emp);
      }

   found = tree_find(tree, 2);
   if (found)
     printf("it is %s\n", (char *) found->name);

   tree_free(tree);
}

static int
_cmp_cb(void *first, void *second)
{
   const char *s1, *s2;

   s1 = first;
   s2 = second;

   return strcmp(s1, s2);
}

static char *
_random_word_gen(void)
{
   int i, len, c;
   char str[32];

   len = rand() % 12 + 1;

   for (i = 0; i < len; i++)
     {
      c = rand() % (122 + 1 - 97) + 97;
      str[i] = (char) c;
   }

  str[i] = 0x00;

  return strdup(str);
}

static void
test_list(void)
{
   list_t *l, *list;
   char *item, *text;
   int i, count;

   list = list_new();

   srand(time(NULL));

   for (i = 0; i < 64; i++)
     {
        char *word = _random_word_gen();
        list = list_add(list, word);
     }

   list = list_sort(list, _cmp_cb);

   list = list_reverse(list);

   LIST_FOREACH(list, l, item)
     {
        printf("list member: %s\n", item);
     }

   text = strdup("hello");
   list = list_add(list, text);

   count = list_count(list);
   printf("list count: %d\n", count);
   if (count >= 10)
     {
        printf("10th: %s\n", list_nth(list, 9));
     }

   item = list_find(list, text);
   if (item)
     printf("YES! %s\n", (char *) item);

   list = list_del(list, text);

   list_free(list);
}

static int
_path_add_cb(const char *path, stat_t *st, void *data)
{
   hash_t *hashtable = data;

   hash_add(hashtable, path, strdup(path));

   return 0;
}

static void
test_hash(void)
{
   hash_t *hashtable = hash_new();

   file_path_walk(".", _path_add_cb, hashtable);

   hash_add(hashtable, "mom!", strdup("Nice!"));
   hash_add(hashtable, "yes", strdup("PIES"));

   char *value = hash_find(hashtable, "yes");
   if (value)
     printf("found %s in hashtable\n", value);

   hash_del(hashtable, "yes");

   hash_dump(hashtable);

   hash_free(hashtable);
}

static void
test_exe(void)
{
   int status;
   char *output = exe_response("/bin/ls");
   if (!output) return;

   printf("output: %s\n", output);
   free(output);

   status = exe_shell("ls -la | wc -l");
   printf("exit status =  %d\n", status);

   status = exe_shell("cat 1");
   printf("exit status =  %d\n", status);

   status = exe_shell("nosuchthing");
   printf("exit status =  %d\n", status);

   status = exe_wait("/bin/ls", NULL);
   printf("exit status =  %d\n", status);


   const char *dirname = "hello my friend";

   file_mkdir(dirname);

   char *escaped = file_path_escape(dirname);
   if (escaped)
     {
        buf_t *buf = buf_new();
        buf_append_printf(buf, "ls -la %s", escaped);
        free(escaped);

        status = exe_shell(buf_string_get(buf));
        printf("exit status = %d\n", status);
        buf_free(buf);
     }

   file_remove(dirname);
}

static void
test_system(void)
{
   list_t *list, *l;
   char *item;

   printf("CPU count: %d\n", system_cpu_count());

   list = system_disks_get();

   LIST_FOREACH(list, l, item)
     {
        char *mount = system_disk_mount_point_get(item);
        if (mount)
          {
             printf("dev: %s mounted at %s\n", item, mount);
             free(mount);
          }
        else
          {
             printf("dev: %s not mounted\n", item);
          }
     }

   printf("\n");

   list_free(list);
}

static void
test_file(const char *path)
{
   buf_t *buf;
   char *filename;
   const char *contents;
   ssize_t size;

   if (file_exists(path) && !file_is_directory(path))
     {
        buf = file_contents_get(path);
        contents = buf_string_get(buf);
        if (contents)
          {
             printf("contents:\n%s\n", contents);
          }
        buf_free(buf);

        filename = file_filename_get(path);
        if (filename)
          {
             size = file_size_get(path);
             printf("path's filename is %s and size %ld\n", filename, size);
             free(filename);
          }
     }

   list_t *l, *files = file_ls("/home");
   if (!files) return;

   l = files;
   while (l)
     {
        printf("%s\n", l->data);
        l = list_next(l);
     }
   list_free(files);
}

static void
test_binary_file(const char *path)
{
   buf_t *buf;
   char *checksum;
   ssize_t size;

   if (!file_exists(path) || file_is_directory(path))
     return;

   checksum = file_sha256sum(path);

   size = file_size_get(path);

   buf = file_contents_get(path);

   int i, j;
   unsigned char result[SHA256_DIGEST_LENGTH] = { 0 };
   char sha256[2 * SHA256_DIGEST_LENGTH + 1] = { 0 };
   SHA256_CTX ctx;

   SHA256_Init(&ctx);

   for (i = 0; i < buf->len; i++)
     {
        SHA256_Update(&ctx, (char *) &buf->data[i], 1);
     }

   SHA256_Final(result, &ctx);

   for (i = 0, j = 0; i < SHA256_DIGEST_LENGTH; i++)
     {
        snprintf(&sha256[j], sizeof(sha256), "%02x", (unsigned int) result[i]);
        j += 2;
     }

   printf("checksum1: %s and checksum2: %s\n", checksum, sha256);

   printf("test binary file: %s!\n", (size == buf->len) && strings_match(checksum, sha256) ? "SUCCESS" : "FAIL");

   buf_free(buf);
   free(checksum);
}

/* Probably better than enabling __I_AM_INSANE__ */
static int
_path_del_all_cb(const char *path, stat_t *st, void *data)
{
   file_remove(path);

   return 0;
}

static void
test_file_actions(void)
{
   const char *path = "ahar";
   char *sum1, *sum2;

   printf("file_mkdir returns %d\n", file_mkdir(path));

   if (file_move(path, "tmp") && file_exists("tmp"))
     {
        puts("copy ok!");
        file_move("tmp", path);
     }

   if (file_directory_is_empty(path))
     printf("%s is empty!\n", path);

   stat_t *st = file_stat("/etc/services");
   if (st)
     {
        printf("mtime: %ld size: %ld\n", st->mtime, st->size);
        free(st);
     }

   file_remove(path);

   // instead of file_remove_all()
   file_path_walk(path, _path_del_all_cb, NULL);

   if(!file_copy("/etc/passwd", "/tmp/testfile.txt"))
     return;
   else
     puts("file_copy() success!");

   sum1 = file_sha256sum("/etc/passwd");
   sum2 = file_sha256sum("/tmp/testfile.txt");

   if (strings_match(sum1, sum2))
     {
        printf("%s matches!\n", sum1);
     }

   free(sum1);
   free(sum2);

   sum1 = file_sha512sum("/etc/passwd");
   sum2 = file_sha512sum("/tmp/testfile.txt");

   if (strings_match(sum1, sum2))
     {
        printf("%s matches!\n", sum1);
     }

   free(sum1);
   free(sum2);
}

int
main(void)
{
   /* Start of tests */

   test_hash();

   test_list();

   test_tree();

   test_system();

   test_file("/etc/passwd");

   test_binary_file("tests/data/example.wav");

   test_file_actions();

   test_exe();

   /* End of tests */

   return EXIT_SUCCESS;
}
