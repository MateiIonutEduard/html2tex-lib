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

static void append_string(LaTeXConverter* converter, const char* str) {
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

static void convert_children(LaTeXConverter* converter, HTMLNode* node) {
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

static void convert_node(LaTeXConverter* converter, HTMLNode* node) {
    if (!node) return;

    /* handle text nodes */
    if (!node->tag && node->content) {
        escape_latex(converter, node->content);
        return;
    }

    if (!node->tag) 
        return;

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
    else if (strcmp(node->tag, "a") == 0) {
        char* href = get_attribute(node->attributes,
            "href");

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
        begin_environment(converter, "itemize");
        convert_children(converter, node);

        end_environment(converter, "itemize");
        append_string(converter, "\n");
    }
    else if (strcmp(node->tag, "ol") == 0) {
        begin_environment(converter, "enumerate");
        convert_children(converter, node);

        end_environment(converter, "enumerate");
        append_string(converter, "\n");
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
    else if (strcmp(node->tag, "div") == 0 || strcmp(node->tag, "span") == 0)
        convert_children(converter, node);
    else {
        /* unknown tag, just convert children */
        convert_children(converter, node);
    }
}