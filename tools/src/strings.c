#include "strings.h"
#include <stdlib.h>
#include <stdint.h>

const char *
string_reverse(char *string)
{
   int i, j;
   size_t length = strlen(string);

   for (i = 0, j = length -1; i < j; i++, j--)
     {
        int temp = string[i];
        string[i] = string[j];
        string[j] = temp;
     }

   return string;
}

const char *
int_to_ascii(int base, int number)
{
   int i;
   int negative = 0;
   static char buf[128 + 2];

   i = sizeof(buf)-1;

   if (number == 0)
     {
        buf[0] = '0';
        buf[1] = 0x00;
        return buf;
     }

   if (number < 0 && base == 10)
     {
        number = -number;
        negative = 1;
     }

   while (number != 0 && i > 0)
     {
        int rem = number % base;
        number /= base;
        buf[i--] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
     }

   if (negative)
     buf[i--] = '-';

   return &buf[i + 1];
}

char *
strings_encode_base64(const char *input)
{
   int len = strlen(input);
   int leftover = len % 3;
   char *ret = malloc(((len / 3) * 4) + ((leftover) ? 4 : 0) + 1);
   int n = 0;
   int outlen = 0;
   uint8_t i = 0;
   uint8_t *inp = (uint8_t *)input;
   const char *index = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                       "abcdefghijklmnopqrstuvwxyz"
                       "0123456789+/";

   if (ret == NULL)
     return NULL;

   // Convert each 3 bytes of input to 4 bytes of output.
   len -= leftover;
   for (n = 0; n < len; n += 3)
     {
        i = inp[n] >> 2;
        ret[outlen++] = index[i];

        i = (inp[n] & 0x03) << 4;
        i |= (inp[n + 1] & 0xf0) >> 4;
        ret[outlen++] = index[i];

        i = ((inp[n + 1] & 0x0f) << 2);
        i |= ((inp[n + 2] & 0xc0) >> 6);
        ret[outlen++] = index[i];

        i = (inp[n + 2] & 0x3f);
        ret[outlen++] = index[i];
     }

   // Handle leftover 1 or 2 bytes.
   if (leftover)
     {
        i = (inp[n] >> 2);
        ret[outlen++] = index[i];

        i = (inp[n] & 0x03) << 4;
        if (leftover == 2)
          {
             i |= (inp[n + 1] & 0xf0) >> 4;
             ret[outlen++] = index[i];

             i = ((inp[n + 1] & 0x0f) << 2);
          }
        ret[outlen++] = index[i];
        ret[outlen++] = '=';
        if (leftover == 1)
          ret[outlen++] = '=';
     }

   ret[outlen] = '\0';

   return ret;
}



bool
strings_match(const char *s1, const char *s2)
{
   if (!s1 || !s2)
     return false;

   if (!strcmp(s1, s2))
     return true;

   return false;
}

bool
string_contains(const char *string, const char *search)
{
   return !! strstr(string, search);
}
