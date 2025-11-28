#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This helper function to check if element is inline, required for formatting. */
static int is_inline_element_for_formatting(const char* tag_name) {
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

/* This helper function is used to escape HTML special characters. */
static char* escape_html(const char* text) {
    if (!text) return NULL;

    size_t len = strlen(text);
    size_t new_len = len;

    /* count special characters to determine buffer size */
    for (const char* p = text; *p; p++) {
        if (*p == '<' || *p == '>' || *p == '&' || *p == '"' || *p == '\'') {
            switch (*p) {
            case '<': case '>': new_len += 3; break;
            case '&': new_len += 4; break;
            case '"': new_len += 5; break;
            case '\'': new_len += 5; break;
            }
        }
    }

    char* escaped = malloc(new_len + 1);
    if (!escaped) return NULL;

    char* dest = escaped;
    for (const char* p = text; *p; p++) {
        switch (*p) {
        case '<': strcpy(dest, "&lt;"); dest += 4; break;
        case '>': strcpy(dest, "&gt;"); dest += 4; break;
        case '&': strcpy(dest, "&amp;"); dest += 5; break;
        case '"': strcpy(dest, "&quot;"); dest += 6; break;
        case '\'': strcpy(dest, "&apos;"); dest += 6; break;
        default: *dest++ = *p; break;
        }
    }

    *dest = '\0';
    return escaped;
}

/* Recursive function to write the prettified HTML code. */
static void write_pretty_node(FILE* file, HTMLNode* node, int indent_level) {
    if (!node || !file) return;

    /* create indentation */
    for (int i = 0; i < indent_level; i++)
        fprintf(file, "  ");

    if (node->tag) {
        /* element node */
        fprintf(file, "<%s", node->tag);

        /* write attributes */
        HTMLAttribute* attr = node->attributes;

        while (attr) {
            if (attr->value) {
                char* escaped_value = escape_html(attr->value);
                fprintf(file, " %s=\"%s\"", attr->key, escaped_value ? escaped_value : attr->value);
                if (escaped_value) free(escaped_value);
            }
            else
                fprintf(file, " %s", attr->key);

            attr = attr->next;
        }

        /* check if this element should be inline */
        int is_inline = is_inline_element_for_formatting(node->tag);

        /* check if this is a self-closing tag or has children */
        if (!node->children && !node->content)
            fprintf(file, " />\n");
        else {
            /* write content if present */
            fprintf(file, ">");

            if (node->content) {
                char* escaped_content = escape_html(node->content);

                if (escaped_content) {
                    fprintf(file, "%s", escaped_content);
                    free(escaped_content);
                }
            }

            /* write children with proper indentation */
            if (node->children) {
                if (!is_inline) fprintf(file, "\n");
                HTMLNode* child = node->children;

                while (child) {
                    write_pretty_node(file, child, indent_level + 1);
                    child = child->next;
                }

                if (!is_inline) {
                    for (int i = 0; i < indent_level; i++)
                        fprintf(file, "  ");
                }
            }

            fprintf(file, "</%s>\n", node->tag);
        }
    }
    else {
        /* text node */
        if (node->content) {
            char* escaped_content = escape_html(node->content);

            if (escaped_content) {
                /* check if this is mostly whitespace */
                int all_whitespace = 1;

                for (char* p = escaped_content; *p; p++) {
                    if (!isspace(*p)) {
                        all_whitespace = 0;
                        break;
                    }
                }

                if (!all_whitespace)
                    fprintf(file, "%s\n", escaped_content);
                else
                    /* for whitespace-only nodes, just output a newline */
                    fprintf(file, "\n");

                free(escaped_content);
            }
        }
    }
}

int write_pretty_html(HTMLNode* root, const char* filename) {
    if (!root || !filename) return 0;
    FILE* file = fopen(filename, "w");

    if (!file) {
        fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
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
    char* html_string = malloc(file_size + 1);

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