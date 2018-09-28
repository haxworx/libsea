#include "strings.h"
#include <stdio.h>

int main(void)
{
   char string[] = {'h', 'e', 'l', 'l', 'o', 0x00};
   int number = 0xf00d;

   printf("%d as string is %s\n", number, int_to_ascii(10, number));
   printf("%d as string is %s\n", -number, int_to_ascii(10, -number));
   printf("%d as string is %s\n", number, int_to_ascii(2, number));
   printf("%d as string is %s\n", number, int_to_ascii(16, number));

   puts("BEFORE");
   printf("%s\n", string);

   puts("AFTER");
   printf("%s\n", string_reverse(string));

   if (strings_match("hello", "hello"))
     puts("OK!");
   else
     puts("FAIL!");

   if (string_contains("hello", "llo"))
     puts("OK!");
   else
     puts("FAIL");
}
