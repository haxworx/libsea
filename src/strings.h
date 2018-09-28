#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <stdbool.h>
#include <string.h>

const char *
int_to_ascii(int base, int number);

const char *
string_reverse(char *string);

bool
strings_match(const char *s1, const char *s2);

bool
string_contains(const char *string, const char *search);

char *
strings_encode_base64(const char *input);

#endif
