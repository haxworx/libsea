#include "buf.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

char *
exe_response(const char *command)
{
   FILE *p;
   buf_t *lines;
   char *output;
   char buf[8192];

   p = popen(command, "r");
   if (!p)
     return NULL;

   lines = buf_new();

   while ((fgets(buf, sizeof(buf), p)) != NULL)
     {
        buf_append(lines, buf);
     }

   pclose(p);

   if (lines->len == 0)
     {
        buf_free(lines);
        return NULL;
     }

   output = strdup(buf_string_get(lines));

   buf_free(lines);

   return output;
}

#define _CMD_ARGS_MAX 128

int
exe_wait(const char *command, const char *arguments)
{
   char *args[_CMD_ARGS_MAX];
   const char *start, *end;
   int status;
   pid_t pid;
   int i = 0;

   args[i++] = strdup(command);

   if (arguments)
     start = &arguments[0];
   else
     start = NULL;

   while (start && start[0])
     {
        if (i == _CMD_ARGS_MAX)
          break;

        end = strchr(start, ' ');
        if (!end)
          {
             args[i++] = strdup(start);
             break;
          }

        if (start && end)
          {
             args[i++] = strndup(start, end - start);
          }
        start = end + 1;
     }

   args[i] = NULL;

   pid = fork();
   if (pid < 0)
     {
        return -1;
     }
   else if (pid == 0)
     {
        if (execvp(command, args) == -1)
          _exit(127);
     }

   pid = wait(&status);
   if (!WIFEXITED(status))
     return -1;

   status = WEXITSTATUS(status);

   for (i = 0; args[i] != NULL; i++)
     {
        free(args[i]);
     }

   return status;
}

int
exe_shell(const char *command)
{
   int status;

   status = system(command);
   status >>= 8;

   return status;
}
