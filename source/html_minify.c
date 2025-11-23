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