#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static void write_pretty_node(FILE* file, HTMLNode* node, int indent_level, int is_inline) {
    if (!node || !file) return;
    static const char* indent_str = "  ";

    /* create indentation */
    char* indent = malloc(indent_level * 2 + 1);

    if (indent) {
        for (int i = 0; i < indent_level; i++) {
            indent[i * 2] = ' ';
            indent[i * 2 + 1] = ' ';
        }

        indent[indent_level * 2] = '\0';
    }

    if (node->tag) {
        /* element node */
        if (!is_inline && indent) fprintf(file, "%s", indent);
        fprintf(file, "<%s", node->tag);

        /* write the attributes */
        HTMLAttribute* attr = node->attributes;

        while (attr) {
            if (attr->value) {
                char* escaped_value = escape_html(attr->value);
                fprintf(file, " %s=\"%s\"", attr->key, escaped_value ? escaped_value : attr->value);
                free(escaped_value);
            }
            else
                fprintf(file, " %s", attr->key);

            attr = attr->next;
        }

        /* check if this is a self-closing tag or has children */
        if (!node->children && !node->content) {
            fprintf(file, " />\n");
        }
        else {
            fprintf(file, ">");

            /* check if this should be inline (no newlines) */
            int child_is_inline = is_inline_element(node->tag);

            /* write content if present */
            if (node->content) {
                char* escaped_content = escape_html(node->content);
                fprintf(file, "%s", escaped_content ? escaped_content : node->content);
                free(escaped_content);
            }

            /* write children with proper indentation */
            if (node->children) {
                if (!child_is_inline) fprintf(file, "\n");
                write_pretty_node(file, node->children, indent_level + 1,
                    child_is_inline);

                if (!child_is_inline && indent)
                    fprintf(file, "%s", indent);
            }

            fprintf(file, "</%s>\n", node->tag);
        }
    }
    else {
        /* text node */
        if (node->content) {
            if (!is_inline && indent) {
                fprintf(file, "%s", indent);
            }

            char* escaped_content = escape_html(node->content);

            /* preserve significant whitespace in text nodes */
            if (escaped_content) {
                /* check if this is mostly whitespace */
                int all_whitespace = 1;

                for (char* p = escaped_content; *p; p++) {
                    if (!isspace(*p)) {
                        all_whitespace = 0;
                        break;
                    }
                }

                if (!all_whitespace) {
                    fprintf(file, "%s", escaped_content);
                    if (!is_inline) fprintf(file, "\n");
                }
                else if (!is_inline)
                    fprintf(file, "\n");
            }

            free(escaped_content);
        }
    }

    free(indent);

    /* write siblings */
    if (node->next)
        write_pretty_node(file, node->next, indent_level, is_inline);
}

/* Public function to write prettified HTML code to the output file. */
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
        write_pretty_node(file, child, 1, 0);
        child = child->next;
    }

    /* write HTML footer */
    fprintf(file, "</body>\n</html>\n");

    fclose(file);
    return 1;
}

/* Alternative function that returns prettified HTML as string. */
char* get_pretty_html(HTMLNode* root) {
    if (!root) return NULL;

    /* use a temporary file to build the string */
    char* temp_filename = tmpnam(NULL);
    if (!temp_filename) return NULL;

    if (!html2tex_write_pretty_html(root, temp_filename))
        return NULL;

    /* read the file back into a string */
    FILE* file = fopen(temp_filename, "r");

    if (!file) return NULL;
    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* html_string = malloc(file_size + 1);

    if (html_string) {
        fread(html_string, 1, file_size, file);
        html_string[file_size] = '\0';
    }

    fclose(file);
    remove(temp_filename); /* clean up temp file */

    return html_string;
}