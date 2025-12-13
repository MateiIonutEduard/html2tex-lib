#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


static void rev_str(char* str, int length) {
    int start = 0;
    int end = length - 1;

    while (start < end) {
        char temp = str[start];
        str[start] = str[end];

        str[end] = temp;
        start++; end--;
    }
}

void portable_itoa(int value, char* buffer, int radix) {
    /* validate inputs */
    if (!buffer || radix < 2 || radix > 36) {
        if (buffer && radix == 10)
            snprintf(buffer, 32, "%d", value);

        return;
    }

    int i = 0;
    int is_negative = 0;
    unsigned int unsigned_value;

    /* handle zero explicitly */
    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    /* handle negative numbers for base 10 only */
    if (radix == 10 && value < 0) {
        is_negative = 1;
        unsigned_value = (unsigned int)(-value);
    }
    else
        /* for other bases, treat as unsigned */
        unsigned_value = (unsigned int)value;

    /* convert number to string in reverse order */
    while (unsigned_value != 0) {
        int remainder = unsigned_value % radix;
        buffer[i++] = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
        unsigned_value /= radix;
    }

    /* add negative sign for base 10 if required */
    if (is_negative) buffer[i++] = '-';
    buffer[i] = '\0';

    /* reverse the string to get correct order */
    rev_str(buffer, i);
}

char* html2tex_strdup(const char* str) {
    if (!str) return NULL;

    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);

    if (copy)
        memcpy(copy, str, len);

    return copy;
}