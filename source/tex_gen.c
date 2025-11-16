#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 1024
#define GROWTH_FACTOR 2

static void ensure_capacity(LaTeXConverter* converter, size_t needed) {
    if (converter->output_capacity == 0) {
        converter->output_capacity = INITIAL_CAPACITY;
        converter->output = malloc(converter->output_capacity);

        converter->output[0] = '\0';
        converter->output_size = 0;
    }

    if (converter->output_size + needed >= converter->output_capacity) {
        converter->output_capacity *= GROWTH_FACTOR;
        converter->output = realloc(converter->output, converter->output_capacity);
    }
}

void append_string(LaTeXConverter* converter, const char* str) {
    size_t len = strlen(str);
    ensure_capacity(converter, len + 1);

    strcat(converter->output, str);
    converter->output_size += len;
}

static void append_char(LaTeXConverter* converter, char c) {
    ensure_capacity(converter, 2);
    converter->output[converter->output_size++] = c;
    converter->output[converter->output_size] = '\0';
}

static char* get_attribute(HTMLAttribute* attrs, const char* key) {
    HTMLAttribute* current = attrs;

    while (current) {
        if (strcmp(current->key, key) == 0)
            return current->value;

        current = current->next;
    }

    return NULL;
}

static void escape_latex(LaTeXConverter* converter, const char* text) {
    if (!text) return;

    for (const char* p = text; *p; p++) {
        switch (*p) {
        case '\\': append_string(converter, "\\textbackslash{}"); break;
        case '{': append_string(converter, "\\{"); break;
        case '}': append_string(converter, "\\}"); break;
        case '&': append_string(converter, "\\&"); break;
        case '%': append_string(converter, "\\%"); break;
        case '$': append_string(converter, "\\$"); break;
        case '#': append_string(converter, "\\#"); break;
        case '_': append_string(converter, "\\_"); break;
        case '^': append_string(converter, "\\^{}"); break;
        case '~': append_string(converter, "\\~{}"); break;
        case '<': append_string(converter, "\\textless{}"); break;
        case '>': append_string(converter, "\\textgreater{}"); break;
        case '\n': append_string(converter, "\\\\"); break;
        default: append_char(converter, *p); break;
        }
    }
}

static void convert_node(LaTeXConverter* converter, HTMLNode* node);

void convert_children(LaTeXConverter* converter, HTMLNode* node) {
    HTMLNode* child = node->children;

    while (child) {
        convert_node(converter, child);
        child = child->next;
    }
}

static void begin_environment(LaTeXConverter* converter, const char* env) {
    append_string(converter, "\\begin{");
    append_string(converter, env);
    append_string(converter, "}\n");
}

static void end_environment(LaTeXConverter* converter, const char* env) {
    append_string(converter, "\\end{");
    append_string(converter, env);
    append_string(converter, "}\n");
}

static char* extract_color_from_style(const char* style, const char* property) {
    if (!style || !property) return NULL;

    char* style_copy = strdup(style);
    char* token = strtok(style_copy, ";");

    while (token) {
        while (*token == ' ') token++;
        char* colon = strchr(token, ':');

        if (colon) {
            *colon = '\0';
            char* prop = token;

            char* value = colon + 1;
            while (*value == ' ') value++;

            if (strcmp(prop, property) == 0) {
                char* result = strdup(value);
                free(style_copy);
                return result;
            }
        }

        token = strtok(NULL, ";");
    }

    free(style_copy);
    return NULL;
}

static char* color_to_hex(const char* color_value) {
    if (!color_value) return NULL;
    char* result = NULL;

    if (color_value[0] == '#')
        result = strdup(color_value + 1);
    else if (strncmp(color_value, "rgb(", 4) == 0) {
        int r, g, b;

        if (sscanf(color_value, "rgb(%d, %d, %d)", &r, &g, &b) == 3) {
            result = malloc(7);
            snprintf(result, 7, "%02X%02X%02X", r, g, b);
        }
    }
    else if (strncmp(color_value, "rgba(", 5) == 0) {
        int r, g, b;
        float a;

        if (sscanf(color_value, "rgba(%d, %d, %d, %f)", &r, &g, &b, &a) == 4) {
            result = malloc(7);
            snprintf(result, 7, "%02X%02X%02X", r, g, b);
        }
    }
    else
        result = strdup(color_value);

    if (result) {
        for (char* p = result; *p; p++)
            *p = toupper(*p);
    }

    return result;
}

/* handle color application with proper nesting */
static void apply_color(LaTeXConverter* converter, const char* color_value, int is_background) {
    if (!color_value) return;

    char* hex_color = color_to_hex(color_value);
    if (!hex_color) return;

    if (is_background) {
        append_string(converter, "\\colorbox[HTML]{");
        append_string(converter, hex_color);
        append_string(converter, "}{");
    }
    else {
        append_string(converter, "\\textcolor[HTML]{");
        append_string(converter, hex_color);
        append_string(converter, "}{");
    }

    free(hex_color);
}

void convert_node(LaTeXConverter* converter, HTMLNode* node) {
    if (!node) return;

    /* handle text nodes - including those with only whitespace */
    if (!node->tag && node->content) {
        escape_latex(converter, node->content);
        return;
    }

    if (!node->tag) return;

    /* handle different HTML tags */
    if (strcmp(node->tag, "p") == 0) {
        append_string(converter, "\n");
        convert_children(converter, node);
        append_string(converter, "\n\n");
    }
    else if (strcmp(node->tag, "h1") == 0) {
        append_string(converter, "\\section{");
        convert_children(converter, node);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "h2") == 0) {
        append_string(converter, "\\subsection{");
        convert_children(converter, node);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "h3") == 0) {
        append_string(converter, "\\subsubsection{");
        convert_children(converter, node);
        append_string(converter, "}\n\n");
    }
    else if (strcmp(node->tag, "b") == 0 || strcmp(node->tag, "strong") == 0) {
        append_string(converter, "\\textbf{");
        convert_children(converter, node);
        append_string(converter, "}");
    }
    else if (strcmp(node->tag, "i") == 0 || strcmp(node->tag, "em") == 0) {
        append_string(converter, "\\textit{");
        convert_children(converter, node);
        append_string(converter, "}");
    }
    else if (strcmp(node->tag, "u") == 0) {
        append_string(converter, "\\underline{");
        convert_children(converter, node);
        append_string(converter, "}");
    }
    else if (strcmp(node->tag, "code") == 0) {
        append_string(converter, "\\texttt{");
        convert_children(converter, node);
        append_string(converter, "}");
    }
    else if (strcmp(node->tag, "font") == 0) {
        /* parse color attribute and style background-color */
        char* color_attr = get_attribute(node->attributes, "color");
        char* style_attr = get_attribute(node->attributes, "style");

        /* direct color attribute */
        char* text_color = color_attr;
        char* bg_color = NULL;

        /* extract background color from style if present */
        if (style_attr)
            bg_color = extract_color_from_style(style_attr, "background-color");

        int has_text_color = (text_color != NULL);
        int has_bg_color = (bg_color != NULL);

        /* apply colors with proper LaTeX nesting */
        if (has_bg_color && has_text_color) {
            /* both colors: background first, then text color */
            apply_color(converter, bg_color, 1);
            apply_color(converter, text_color, 0);

            convert_children(converter, node);
            append_string(converter, "}}");
        }
        else if (has_bg_color) {
            /* only background color */
            apply_color(converter, bg_color, 1);

            convert_children(converter, node);
            append_string(converter, "}");
        }
        else if (has_text_color) {
            /* only text color */
            apply_color(converter, text_color, 0);

            convert_children(converter, node);
            append_string(converter, "}");
        }
        else {
            /* no color specified, just convert content */
            convert_children(converter, node);
        }

        /* clean up allocated memory */
        if (bg_color) free(bg_color);
    }
    else if (strcmp(node->tag, "span") == 0) {
        /* handle span tags with inline styles */
        char* style_attr = get_attribute(node->attributes, "style");

        if (style_attr) {
            char* text_color = extract_color_from_style(style_attr, "color");
            char* bg_color = extract_color_from_style(style_attr, "background-color");

            int has_text_color = (text_color != NULL);
            int has_bg_color = (bg_color != NULL);

            /* apply colors */
            if (has_bg_color && has_text_color) {
                apply_color(converter, bg_color, 1);
                apply_color(converter, text_color, 0);

                convert_children(converter, node);
                append_string(converter, "}}");
            }
            else if (has_bg_color) {
                apply_color(converter, bg_color, 1);
                convert_children(converter, node);
                append_string(converter, "}");
            }
            else if (has_text_color) {
                apply_color(converter, text_color, 0);
                convert_children(converter, node);
                append_string(converter, "}");
            }
            else
                convert_children(converter, node);

            /* clean up */
            if (text_color) free(text_color);
            if (bg_color) free(bg_color);
        }
        else
            convert_children(converter, node);
    }
    else if (strcmp(node->tag, "a") == 0) {
        char* href = get_attribute(node->attributes, "href");

        if (href) {
            append_string(converter, "\\href{");
            escape_latex(converter, href);
            append_string(converter, "}{");

            convert_children(converter, node);
            append_string(converter, "}");
        }
        else
            convert_children(converter, node);
    }
    else if (strcmp(node->tag, "ul") == 0) {
        append_string(converter, "\\begin{itemize}\n");
        convert_children(converter, node);
        append_string(converter, "\\end{itemize}\n");
    }
    else if (strcmp(node->tag, "ol") == 0) {
        append_string(converter, "\\begin{enumerate}\n");
        convert_children(converter, node);
        append_string(converter, "\\end{enumerate}\n");
    }
    else if (strcmp(node->tag, "li") == 0) {
        append_string(converter, "\\item ");
        convert_children(converter, node);
        append_string(converter, "\n");
    }
    else if (strcmp(node->tag, "br") == 0)
        append_string(converter, "\\\\\n");
    else if (strcmp(node->tag, "hr") == 0)
        append_string(converter, "\\hrulefill\n\n");
    else if (strcmp(node->tag, "div") == 0)
        convert_children(converter, node);
    else {
        /* unknown tag, just convert children */
        convert_children(converter, node);
    }
}