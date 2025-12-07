#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* This helper function to check if element is inline, required for formatting. */
static int is_inline_element_for_formatting(const char* tag_name) {
    if (!tag_name) return 0;

    static const char* const inline_tags[] = {
        "a", "abbr", "b", "bdi", "bdo", "br", "cite", "code",
        "data", "dfn", "em", "font", "i", "kbd", "mark", "q",
        "rp", "rt", "ruby", "samp", "small", "span", "strong",
        "sub", "sup", "time", "u", "var", "wbr", NULL
    };

    /* fast check for single-char tags */
    if (tag_name[1] == '\0') {
        char c = tag_name[0];

        return c == 'a' || c == 'b' 
            || c == 'i' || c == 'u' 
            || c == 'q' || c == 's' 
            || c == 'v';
    }

    /* optimize linear search with length check */
    size_t len = strlen(tag_name);

    for (int i = 0; inline_tags[i]; i++) {
        /* fast reject by length first */
        if (strlen(inline_tags[i]) != len) continue;

        /* compare the rest of tag name */
        if (strcmp(tag_name, inline_tags[i]) == 0)
            return 1;
    }

    return 0;
}

/* This helper function is used to escape HTML special characters. */
static char* escape_html(const char* text) {
    if (!text) return NULL;
    const char* p = text;
    size_t extra = 0;

    /* count special chars efficiently */
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (c == '<' || c == '>') extra += 3;

        else if (c == '&') {
            /* check if already escaped */
            if (strncmp(p, "&lt;", 4) && strncmp(p, "&gt;", 4) &&
                strncmp(p, "&amp;", 5) && strncmp(p, "&quot;", 6) &&
                strncmp(p, "&apos;", 6) && strncmp(p, "&#", 2))
                extra += 4;
        }

        else if (c == '"') extra += 5;
        else if (c == '\'') extra += 5;
        p++;
    }

    /* no escaping needed, return copy */
    if (extra == 0) return strdup(text);

    /* allocate once */
    size_t len = p - text;

    char* escaped = (char*)malloc(len + extra + 1);
    if (!escaped) return NULL;

    /* copy with escaping */
    char* dest = escaped;
    p = text;

    while (*p) {
        unsigned char c = (unsigned char)*p;

        if (c == '<') {
            memcpy(dest, "&lt;", 4);
            dest += 4;
        }
        else if (c == '>') {
            memcpy(dest, "&gt;", 4);
            dest += 4;
        }
        else if (c == '&') {
            /* check if already escaped */
            if (strncmp(p, "&lt;", 4) && strncmp(p, "&gt;", 4) &&
                strncmp(p, "&amp;", 5) && strncmp(p, "&quot;", 6) &&
                strncmp(p, "&apos;", 6) && strncmp(p, "&#", 2)) {
                memcpy(dest, "&amp;", 5);
                dest += 5;
            }
            else
                *dest++ = *p;
        }
        else if (c == '"') {
            memcpy(dest, "&quot;", 6);
            dest += 6;
        }
        else if (c == '\'') {
            memcpy(dest, "&apos;", 6);
            dest += 6;
        }
        else
            *dest++ = *p;
        p++;
    }

    *dest = '\0';
    return escaped;
}

/* Recursive function to write the prettified HTML code. */
static void write_pretty_node(FILE* file, HTMLNode* node, int indent_level) {
    if (!node || !file) return;

    /* use fixed size buffer for efficient writing */
    static char buffer[8192];
    static size_t buf_pos = 0;

    /* helper macros used for buffered writes */
#define FLUSH_BUFFER() \
        if (buf_pos > 0) { \
            fwrite(buffer, 1, buf_pos, file); \
            buf_pos = 0; \
        }

#define CHECK_BUFFER(needed) \
        if (buf_pos + (needed) >= sizeof(buffer)) FLUSH_BUFFER()

#define BUFFER_WRITE(str, len) \
        do { \
            CHECK_BUFFER(len); \
            memcpy(buffer + buf_pos, (str), (len)); \
            buf_pos += (len); \
        } while(0)

#define BUFFER_PRINTF(fmt, ...) \
        do { \
            int needed = snprintf(NULL, 0, fmt, __VA_ARGS__); \
            CHECK_BUFFER(needed + 1); \
            buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, fmt, __VA_ARGS__); \
        } while(0)

    /* indentation */
    for (int i = 0; i < indent_level; i++)
        BUFFER_WRITE("  ", 2);

    if (node->tag) {
        /* node element */
        BUFFER_PRINTF("<%s", node->tag);

        /* write attributes */
        HTMLAttribute* attr = node->attributes;

        while (attr) {
            if (attr->value) {
                char* escaped_value = escape_html(attr->value);

                if (escaped_value) {
                    BUFFER_PRINTF(" %s=\"%s\"", attr->key, escaped_value);
                    free(escaped_value);
                }
                else
                    BUFFER_PRINTF(" %s=\"%s\"", attr->key, attr->value);
            }
            else
                BUFFER_PRINTF(" %s", attr->key);
            attr = attr->next;
        }

        /* check if inline */
        int is_inline = is_inline_element_for_formatting(node->tag);

        /* check if self-closing */
        if (!node->children && !node->content)
            BUFFER_WRITE(" />\n", 4);
        else {
            /* write content if present */
            BUFFER_WRITE(">", 1);

            if (node->content) {
                char* escaped_content = escape_html(node->content);

                if (escaped_content) {
                    /* check if it is whitespace only */
                    int all_whitespace = 1;

                    for (char* p = escaped_content; *p; p++) {
                        if (!isspace((unsigned char)*p)) {
                            all_whitespace = 0;
                            break;
                        }
                    }

                    if (!all_whitespace)
                        BUFFER_PRINTF("%s", escaped_content);
                    free(escaped_content);
                }
            }

            /* write children elements */
            if (node->children) {
                if (!is_inline)
                    BUFFER_WRITE("\n", 1);

                /* flush before recursion */
                FLUSH_BUFFER();
                HTMLNode* child = node->children;

                while (child) {
                    write_pretty_node(file, child, indent_level + 1);
                    child = child->next;
                }

                if (!is_inline) {
                    /* indentation required for closing tag */
                    for (int i = 0; i < indent_level; i++)
                        BUFFER_WRITE("  ", 2);
                }
            }

            BUFFER_PRINTF("</%s>\n", node->tag);
        }
    }
    else {
        /* text node element */
        if (node->content) {
            char* escaped_content = escape_html(node->content);

            if (escaped_content) {
                /* check if whitespace-only */
                int all_whitespace = 1;

                for (char* p = escaped_content; *p; p++) {
                    if (!isspace((unsigned char)*p)) {
                        all_whitespace = 0;
                        break;
                    }
                }

                if (!all_whitespace)
                    BUFFER_PRINTF("%s\n", escaped_content);
                else
                    BUFFER_WRITE("\n", 1);
                free(escaped_content);
            }
        }
    }

    /* final flush if we are at the top level */
    if (indent_level == 0)
        FLUSH_BUFFER();

#undef FLUSH_BUFFER
#undef CHECK_BUFFER
#undef BUFFER_WRITE
#undef BUFFER_PRINTF
}

int write_pretty_html(HTMLNode* root, const char* filename) {
    if (!root || !filename) return 0;
    FILE* file = fopen(filename, "w");

    if (!file) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return 0;
    }

    /* write HTML header */
    fprintf(file, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(file, "  <meta charset=\"UTF-8\">\n");
    fprintf(file, "  <title>Parsed HTML Output</title>\n");
    fprintf(file, "</head>\n<body>\n");

    /* write the parsed content */
    HTMLNode* child = root->children;

    while (child) {
        write_pretty_node(file, child, 1);
        child = child->next;
    }

    /* write HTML footer */
    fprintf(file, "</body>\n</html>\n");

    fclose(file);
    return 1;
}

char* get_pretty_html(HTMLNode* root) {
    if (!root) return NULL;

    /* use a temporary file to build the string */
    char* temp_filename = tmpnam(NULL);
    if (!temp_filename) return NULL;

    if (!write_pretty_html(root, temp_filename))
        return NULL;

    /* read the file back into a string */
    FILE* file = fopen(temp_filename, "rb");

    if (!file) {
        remove(temp_filename);
        return NULL;
    }

    /* get file size more reliably */
    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        remove(temp_filename);
        return strdup("");
    }

    /* allocate and read file content */
    char* html_string = (char*)malloc(file_size + 1);

    if (!html_string) {
        fclose(file);
        remove(temp_filename);
        return NULL;
    }

    size_t bytes_read = fread(html_string, 1, file_size, file);
    html_string[bytes_read] = '\0';

    fclose(file); remove(temp_filename);
    return html_string;
}