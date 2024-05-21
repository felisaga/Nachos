#include "syscall.h"

#define NULL  ((void *) 0)

unsigned strlen (const char *s) {
  unsigned i;
  for (i = 0; s[i] != '\0'; i++);
  return i;
}

void ourPuts(const char *s) {
  OpenFileId output = CONSOLE_OUTPUT;
  Write(s, strlen(s), output);
}

void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}

void itoa (int n , char *str) {
    if(str == NULL)
        return;

    unsigned i = 0;

    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    while (n != 0){
        int rem = n % 10;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / 10;
    }

    if(n < 0) str[i++] = '-';

    str[i] = '\0';
 
    reverse(str, i);
    return;
}
