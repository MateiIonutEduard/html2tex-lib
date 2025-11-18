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

/* check if CSS value contains !important */
static int has_important(const char* value) {
    if (!value) return 0;
    return (strstr(value, "!important") != NULL);
}

/* parse CSS style string into structured properties with !important support */
CSSProperties* parse_css_style(const char* style_str) {
    if (!style_str) return NULL;
    CSSProperties* props = calloc(1, sizeof(CSSProperties));

    if (!props) return NULL;
    char* copy = strdup(style_str);

    if (!copy) {
        free(props);
        return NULL;
    }

    char* token = strtok(copy, ";");

    while (token) {
        /* trim whitespace */
        while (*token == ' ') token++;
        char* colon = strchr(token, ':');

        if (colon) {
            *colon = '\0';
            char* property = token;
            char* value = colon + 1;

            /* trim value and remove !important */
            char* cleaned_value = clean_css_value(value);
            if (!cleaned_value) {
                token = strtok(NULL, ";");
                continue;
            }

            /* map CSS properties */
            if (strcmp(property, "font-weight") == 0)
                props->font_weight = strdup(cleaned_value);
            else if (strcmp(property, "font-style") == 0)
                props->font_style = strdup(cleaned_value);
            else if (strcmp(property, "font-family") == 0)
                props->font_family = strdup(cleaned_value);
            else if (strcmp(property, "font-size") == 0)
                props->font_size = strdup(cleaned_value);
            else if (strcmp(property, "color") == 0)
                props->color = strdup(cleaned_value);
            else if (strcmp(property, "background-color") == 0)
                props->background_color = strdup(cleaned_value);
            else if (strcmp(property, "text-align") == 0)
                props->text_align = strdup(cleaned_value);
            else if (strcmp(property, "text-decoration") == 0)
                props->text_decoration = strdup(cleaned_value);
            else if (strcmp(property, "margin-top") == 0)
                props->margin_top = strdup(cleaned_value);
            else if (strcmp(property, "margin-bottom") == 0)
                props->margin_bottom = strdup(cleaned_value);
            else if (strcmp(property, "margin-left") == 0)
                props->margin_left = strdup(cleaned_value);
            else if (strcmp(property, "margin-right") == 0)
                props->margin_right = strdup(cleaned_value);
            else if (strcmp(property, "padding-top") == 0)
                props->padding_top = strdup(cleaned_value);
            else if (strcmp(property, "padding-bottom") == 0)
                props->padding_bottom = strdup(cleaned_value);
            else if (strcmp(property, "padding-left") == 0)
                props->padding_left = strdup(cleaned_value);
            else if (strcmp(property, "padding-right") == 0)
                props->padding_right = strdup(cleaned_value);
            else if (strcmp(property, "width") == 0)
                props->width = strdup(cleaned_value);
            else if (strcmp(property, "height") == 0)
                props->height = strdup(cleaned_value);
            else if (strcmp(property, "border") == 0)
                props->border = strdup(cleaned_value);
            else if (strcmp(property, "border-color") == 0)
                props->border_color = strdup(cleaned_value);
            else if (strcmp(property, "display") == 0)
                props->display = strdup(cleaned_value);
            else if (strcmp(property, "float") == 0)
                props->float_pos = strdup(cleaned_value);
            else if (strcmp(property, "vertical-align") == 0)
                props->vertical_align = strdup(cleaned_value);

            free(cleaned_value);
        }

        token = strtok(NULL, ";");
    }

    free(copy);
    return props;
}