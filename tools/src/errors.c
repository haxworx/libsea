#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
errors_fail(const char *reason)
{
   fprintf(stderr, "Error: %s\n", reason);
   exit(EXIT_FAILURE);
}

