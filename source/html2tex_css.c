#include "html2tex.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <math.h>

#define HT_MAX_CSS_PROPERTIES 50

/* helper function to remove !important and trim whitespace */
static char* clean_css_value(char* value) {
    if (!value) return NULL;

    char* cleaned = strdup(value);
    if (!cleaned) return NULL;

    /* remove !important */
    char* important_pos = strstr(cleaned, "!important");
    if (important_pos) *important_pos = '\0';

    /* trim trailing whitespace */
    char* end = cleaned + strlen(cleaned) - 1;

    while (end > cleaned && isspace(*end))
        *end-- = '\0';

    /* trim leading whitespace */
    char* start = cleaned;

    while (*start && isspace(*start))
        start++;

    if (start != cleaned)
        memmove(cleaned, start, strlen(start) + 1);

    return cleaned;
}