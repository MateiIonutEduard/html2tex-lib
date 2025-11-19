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

            /* handle !important in CSS values */
            char* important_pos = strstr(value, "!important");

            if (important_pos) {
                /* trim trailing whitespace */
                *important_pos = '\0';

                char* end = value + strlen(value) - 1;
                while (end > value && isspace(*end)) *end-- = '\0';
            }

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

static void begin_table(LaTeXConverter* converter, int columns) {
    converter->state.table_counter++;
    converter->state.in_table = 1;

    converter->state.table_columns = columns;
    converter->state.current_column = 0;
    converter->state.table_caption = NULL;

    append_string(converter, "\\begin{table}[h]\n\\centering\n");
    append_string(converter, "\\begin{tabular}{|");

    /* add vertical lines between columns */
    for (int i = 0; i < columns; i++)
        append_string(converter, "c|");

    append_string(converter, "}\n\\hline\n");
}

static void end_table(LaTeXConverter* converter) {
    if (converter->state.in_table) {
        append_string(converter, "\\end{tabular}\n");

        if (converter->state.table_caption) {
            /* output the pre-formatted caption without escaping */
            append_string(converter, "\\caption{");

            append_string(converter, converter->state.table_caption);
            append_string(converter, "}\n");

            free(converter->state.table_caption);
            converter->state.table_caption = NULL;
        }
        else {
            /* fallback to default caption */
            append_string(converter, "\\caption{Table ");

            char counter_str[16];
            snprintf(counter_str, sizeof(counter_str), "%d", converter->state.table_counter);

            append_string(converter, counter_str);
            append_string(converter, "}\n");
        }

        append_string(converter, "\\end{table}\n\n");
    }

    converter->state.in_table = 0;
    converter->state.in_table_row = 0;
    converter->state.in_table_cell = 0;
}

static void begin_table_row(LaTeXConverter* converter) {
    converter->state.in_table_row = 1;
    converter->state.current_column = 0;
}

static void end_table_row(LaTeXConverter* converter) {
    if (converter->state.in_table_row) {
        append_string(converter, " \\\\ \\hline\n");
        converter->state.in_table_row = 0;
    }
}

static void begin_table_cell(LaTeXConverter* converter, int is_header) {
    converter->state.in_table_cell = 1;
    converter->state.current_column++;

    if (converter->state.current_column > 1)
        append_string(converter, " & ");

    if (is_header)
        append_string(converter, "\\textbf{");
}

static void end_table_cell(LaTeXConverter* converter, int is_header) {
    if (is_header)
        append_string(converter, "}");

    converter->state.in_table_cell = 0;
}

static int count_table_columns(HTMLNode* node) {
    if (!node) return 1;
    int max_columns = 0;

    /* look through all direct children to find rows */
    HTMLNode* child = node->children;

    while (child) {
        if (child->tag) {
            if (child->tag) {
                /* skip caption elements when counting columns */
                if (strcmp(child->tag, "caption") == 0) {
                    child = child->next;
                    continue;
                }
            }

            if (strcmp(child->tag, "tr") == 0) {
                /* this is a row - count its cells */
                int row_columns = 0;
                HTMLNode* cell = child->children;

                while (cell) {
                    if (cell->tag && (strcmp(cell->tag, "td") == 0 || strcmp(cell->tag, "th") == 0)) {
                        char* colspan_attr = get_attribute(cell->attributes, "colspan");
                        int colspan = 1;

                        if (colspan_attr) {
                            colspan = atoi(colspan_attr);
                            if (colspan < 1) colspan = 1;
                        }

                        row_columns += colspan;
                    }

                    cell = cell->next;
                }

                if (row_columns > max_columns)
                    max_columns = row_columns;
            }
            else if (strcmp(child->tag, "thead") == 0 || strcmp(child->tag, "tbody") == 0 || strcmp(child->tag, "tfoot") == 0) {
                /* recursively count columns in table sections */
                int section_columns = count_table_columns(child);

                if (section_columns > max_columns)
                    max_columns = section_columns;
            }
        }

        child = child->next;
    }

    return max_columns > 0 ? max_columns : 1;
}

/* new function to extract caption text */
static char* extract_caption_text(HTMLNode* node) {
    if (!node) return NULL;
    size_t buffer_size = 256;

    char* buffer = malloc(buffer_size);
    if (!buffer) return NULL;

    buffer[0] = '\0';
    size_t current_length = 0;

    /* process the current node and its children, but not siblings */
    if (!node->tag && node->content) {
        /* this is a text node */
        size_t content_len = strlen(node->content);

        if (current_length + content_len + 1 > buffer_size) {
            buffer_size = (current_length + content_len + 1) * 2;
            char* new_buffer = realloc(buffer, buffer_size);

            if (!new_buffer) {
                free(buffer);
                return NULL;
            }

            buffer = new_buffer;
        }

        strcat(buffer, node->content);
        current_length += content_len;
    }

    /* process children recursively */
    if (node->children) {
        char* child_text = extract_caption_text(node->children);

        if (child_text) {
            size_t child_len = strlen(child_text);

            if (current_length + child_len + 1 > buffer_size) {
                buffer_size = (current_length + child_len + 1) * 2;
                char* new_buffer = realloc(buffer, buffer_size);

                if (!new_buffer) {
                    free(buffer);
                    free(child_text);
                    return NULL;
                }

                buffer = new_buffer;
            }

            strcat(buffer, child_text);
            current_length += child_len;
            free(child_text);
        }
    }

    return buffer;
}

void convert_node(LaTeXConverter* converter, HTMLNode* node) {
    if (!node) return;

    /* handle text nodes - including those with only whitespace */
    if (!node->tag && node->content) {
        escape_latex(converter, node->content);
        return;
    }

    if (!node->tag) return;

    // CSS properties parsing and application
    CSSProperties* css_props = NULL;

    // skip CSS processing for caption nodes in tables to prevent state leakage
    if (!(converter->state.in_table && node->tag && strcmp(node->tag, "caption") == 0)) {
        char* style_attr = get_attribute(node->attributes, "style");
        if (style_attr) css_props = parse_css_style(style_attr);

        /* apply CSS properties before element content */
        if (css_props) apply_css_properties(converter, css_props, node->tag);
    }

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
        /* only apply bold if CSS hasn't already applied it */
        if (!converter->state.has_bold) {
            append_string(converter, "\\textbf{");
            convert_children(converter, node);
            append_string(converter, "}");
        }
        else {
            /* CSS already applied bold, just convert children */
            convert_children(converter, node);
            /* don't reset the bold flag here - let the parent element handle it */
            /* this prevents premature resetting of CSS state */
        }
    }
    else if (strcmp(node->tag, "i") == 0 || strcmp(node->tag, "em") == 0) {
        /* only apply italic if CSS hasn't already applied it */
        if (!converter->state.has_italic) {
            append_string(converter, "\\textit{");
            convert_children(converter, node);
            append_string(converter, "}");
        }
        else {
            /* CSS already applied italic, just convert children */
            convert_children(converter, node);

            /* reset the italic flag */
            converter->state.has_italic = 0;
        }
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
        char* text_color = NULL;

        /* extract text color from style if present */
        if (style_attr)
            text_color = extract_color_from_style(style_attr, "color");

        if (css_props && text_color) {
            /* ignore the color attribute, just convert content */
            convert_children(converter, node);
        }
        else if (css_props && !text_color) {
            /* inline CSS exists, but do not contain color property */
            if (color_attr) apply_color(converter, color_attr, 0);

            convert_children(converter, node);
            append_string(converter, "}");
        }

        /* clean up allocated memory */
        if (text_color) free(text_color);
    }
    else if (strcmp(node->tag, "span") == 0)
        /* CSS properties handle styling, just convert content */
        convert_children(converter, node);
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
    /* table support */
    else if (strcmp(node->tag, "table") == 0) {
        /* reset CSS state before table */
        reset_css_state(converter);

        int columns = count_table_columns(node);
        begin_table(converter, columns);

        /* convert all children including caption */
        HTMLNode* child = node->children;

        while (child) {
            convert_node(converter, child);
            child = child->next;
        }

        /* reset CSS state after table */
        end_table(converter);
        reset_css_state(converter);
    }
    // added explicit caption handling
    else if (strcmp(node->tag, "caption") == 0) {
        /* handle table caption */
        if (converter->state.in_table) {
            /* free any existing caption */
            if (converter->state.table_caption) {
                free(converter->state.table_caption);
                converter->state.table_caption = NULL;
            }

            /* skip CSS processing for caption to prevent state leakage */
            /* extract raw caption text */
            char* raw_caption = extract_caption_text(node);

            if (raw_caption) {
                /* parse CSS properties separately without affecting converter state */
                CSSProperties* css_props = NULL;

                char* style_attr = get_attribute(node->attributes, "style");
                if (style_attr) css_props = parse_css_style(style_attr);

                /* apply CSS formatting directly to caption without converter */
                if (css_props) {
                    size_t buffer_size = strlen(raw_caption) * 2 + 256;
                    char* formatted_caption = malloc(buffer_size);

                    if (formatted_caption) {
                        formatted_caption[0] = '\0';

                        /* apply color if present */
                        if (css_props->color) {
                            char* hex_color = css_color_to_hex(css_props->color);
                            if (hex_color && strcmp(hex_color, "000000") != 0) {
                                strcat(formatted_caption, "\\textcolor[HTML]{");
                                strcat(formatted_caption, hex_color);
                                strcat(formatted_caption, "}{");
                                free(hex_color);
                            }
                        }

                        /* apply bold if present */
                        int has_bold = 0;
                        if (css_props->font_weight &&
                            (strcmp(css_props->font_weight, "bold") == 0 ||
                                strcmp(css_props->font_weight, "bolder") == 0)) {
                            strcat(formatted_caption, "\\textbf{");
                            has_bold = 1;
                        }

                        /* add the actual caption text */
                        strcat(formatted_caption, raw_caption);

                        /* close formatting braces in reverse order */
                        if (has_bold)
                            strcat(formatted_caption, "}");

                        if (css_props->color) {
                            char* hex_color = css_color_to_hex(css_props->color);

                            if (hex_color && strcmp(hex_color, "000000") != 0) {
                                strcat(formatted_caption, "}");
                                free(hex_color);
                            }
                        }

                        converter->state.table_caption = formatted_caption;
                    }
                    free_css_properties(css_props);
                }
                else
                    /* no CSS, just use raw caption */
                    converter->state.table_caption = raw_caption;
            }

            /* skip children conversion for caption in table */
            /* we already extracted the text, so don't process children normally */
            return;
        }
        else {
            /* If not in table, convert as normal text */
            convert_children(converter, node);
        }
    }
    else if (strcmp(node->tag, "thead") == 0 || strcmp(node->tag, "tbody") == 0 || strcmp(node->tag, "tfoot") == 0)
        convert_children(converter, node);
    else if (strcmp(node->tag, "tr") == 0) {
        /* reset CSS state for each row */
        reset_css_state(converter);
        converter->state.current_column = 0;

        /* reset any pending CSS state before starting new row */
        converter->state.css_braces = 0;
        converter->state.css_environments = 0;

        converter->state.pending_margin_bottom = 0;
        begin_table_row(converter);

        convert_children(converter, node);
        end_table_row(converter);
    }
    else if (strcmp(node->tag, "td") == 0 || strcmp(node->tag, "th") == 0) {
        int is_header = (strcmp(node->tag, "th") == 0);

        /* handle colspan */
        char* colspan_attr = get_attribute(node->attributes, "colspan");
        int colspan = 1;

        if (colspan_attr) {
            colspan = atoi(colspan_attr);
            if (colspan < 1) colspan = 1;
        }

        /* add column separator if needed */
        if (converter->state.current_column > 0)
            append_string(converter, " & ");

        /* save current CSS brace count before processing this cell */
        int saved_css_braces = converter->state.css_braces;

        /* apply CSS properties first - this will handle cellcolor for table cells */
        if (css_props) apply_css_properties(converter, css_props, node->tag);

        /* handle header formatting - only if CSS hasn't already applied bold */
        if (is_header && !converter->state.has_bold)
            append_string(converter, "\\textbf{");

        /* convert cell content */
        converter->state.in_table_cell = 1;
        convert_children(converter, node);
        converter->state.in_table_cell = 0;

        /* end header formatting */
        if (is_header && !converter->state.has_bold)
            append_string(converter, "}");

        /* close any braces that were opened by CSS for this specific cell */
        int braces_opened_in_this_cell = converter->state.css_braces - saved_css_braces;
        for (int i = 0; i < braces_opened_in_this_cell; i++) {
            append_string(converter, "}");
        }
        converter->state.css_braces = saved_css_braces;

        /* end CSS properties after cell content but BEFORE column separators */
        if (css_props) {
            end_css_properties(converter, css_props, node->tag);
            free_css_properties(css_props);
            css_props = NULL; /* prevent double-free later */
        }

        /* update column count for colspan */
        converter->state.current_column += colspan;

        /* add empty placeholders for additional colspan columns */
        for (int i = 1; i < colspan; i++) {
            converter->state.current_column++;
            if (converter->state.current_column > 0)
                append_string(converter, " & ");

            /* empty cell for colspan */
            append_string(converter, " ");
        }
        }
    else {
        /* unknown tag, just convert children */
        convert_children(converter, node);
    }

    /* end CSS properties after element content - but skip for table cells since we handle them separately */
    if (css_props) {
        if (!(strcmp(node->tag, "td") == 0 || strcmp(node->tag, "th") == 0))
            end_css_properties(converter, css_props, node->tag);
        
        free_css_properties(css_props);
    }
}