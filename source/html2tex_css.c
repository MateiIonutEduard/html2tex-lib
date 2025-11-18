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

/* check if element is block-level */
int is_block_element(const char* tag_name) {
    if (!tag_name) return 0;

    const char* block_tags[] = {
        "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
        "ul", "ol", "li", "table", "tr", "td", "th",
        "blockquote", "section", "article", "header", "footer",
        "nav", "aside", "main", "figure", "figcaption", NULL
    };

    for (int i = 0; block_tags[i]; i++) {
        if (strcmp(tag_name, block_tags[i]) == 0)
            return 1;
    }

    return 0;
}

/* check if element is inline */
int is_inline_element(const char* tag_name) {
    if (!tag_name) return 0;

    const char* inline_tags[] = {
        "span", "a", "strong", "em", "b", "i", "u", "code",
        "font", "mark", "small", "sub", "sup", "time", NULL
    };

    for (int i = 0; inline_tags[i]; i++) {
        if (strcmp(tag_name, inline_tags[i]) == 0)
            return 1;
    }

    return 0;
}