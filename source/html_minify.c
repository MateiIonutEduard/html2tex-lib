#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "html2tex.h"

/* Check whether a tag is safe to minify by removing surrounding whitespace. */
static int is_safe_to_minify_tag(const char* tag_name) {
    if (!tag_name) return 0;

    /* tags where whitespace is significant */
    const char* preserve_whitespace_tags[] = {
        "pre", "code", "textarea", "script", "style", NULL
    };

    for (int i = 0; preserve_whitespace_tags[i]; i++) {
        if (strcmp(tag_name, preserve_whitespace_tags[i]) == 0)
            return 0;
    }

    return 1;
}

/* Check if text content is only whitespace. */
static int is_whitespace_only(const char* text) {
    if (!text) return 1;

    for (const char* p = text; *p; p++) {
        if (!isspace(*p))
            return 0;
    }

    return 1;
}

/* Remove the unnecessary whitespace from text content. */
static char* minify_text_content(const char* text, int is_in_preformatted) {
    if (!text || is_in_preformatted)
        return text ? strdup(text) : NULL;

    /* for normal text, collapse multiple whitespace into single space */
    char* result = malloc(strlen(text) + 1);
    if (!result) return NULL;

    char* dest = result;
    int in_whitespace = 0;

    for (const char* src = text; *src; src++) {
        if (isspace(*src)) {
            if (!in_whitespace && dest != result) {
                *dest++ = ' ';
                in_whitespace = 1;
            }
        }
        else {
            *dest++ = *src;
            in_whitespace = 0;
        }
    }

    *dest = '\0';

    /* trim leading/trailing whitespace */
    if (dest > result && isspace(*(dest - 1)))
        *(dest - 1) = '\0';

    /* if result is empty after trimming, return NULL */
    if (result[0] == '\0') {
        free(result);
        return NULL;
    }

    return result;
}