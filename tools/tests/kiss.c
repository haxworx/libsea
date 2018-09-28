#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "kiss.h"

int
main(void)
{
   char *space = NULL;
   FILE *fp = NULL;
   int status = 1;

   INFO("Hello");
   ALLOC(space, 1000);
   ALLOC(space, 10000);
   C_CHECK(fp = fopen("kiss.c", "r"));
error:
   FCLOSE(fp);
   ALLOC(space, 0);
   INFO("Bye");
   exit(status);
}

