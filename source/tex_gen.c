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

static void escape_latex_special(LaTeXConverter* converter, const char* text)
{
    if (!text) return;

    for (const char* p = text; *p; p++) {
        switch (*p) {
        case '{': append_string(converter, "\\{"); break;
        case '}': append_string(converter, "\\}"); break;
        case '&': append_string(converter, "\\&"); break;
        case '%': append_string(converter, "\\%"); break;
        case '$': append_string(converter, "\\$"); break;
        case '#': append_string(converter, "\\#"); break;
        case '^': append_string(converter, "\\^{}"); break;
        case '~': append_string(converter, "\\~{}"); break;
        case '<': append_string(converter, "\\textless{}"); break;
        case '>': append_string(converter, "\\textgreater{}"); break;
        case '\n': append_string(converter, "\\\\"); break;
        default: append_char(converter, *p); break;
        }
    }
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

static void end_table(LaTeXConverter* converter, const char* table_label) {
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

        if (table_label) {
            append_string(converter, "\\label{tab:");
            escape_latex_special(converter, table_label);
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

/* Helper function to check if a tag should be excluded from conversion. */
static int should_exclude_tag(const char* tag_name) {
    if (!tag_name) return 0;

    const char* excluded_tags[] = {
        "script", "style", "link", "meta", "head",
        "noscript", "template", "iframe", "form",
        "input", "label", "canvas", "svg", "video",
        "source", "audio", "object", "button", "map",
        "area", "frame", "frameset", "noframes", "nav",
        "picture", "progress", "select", "option", "param",
        "search", "samp", "track", "var", "wbr", "mark",
        "meter", "optgroup", "q", "blockquote", "bdo", NULL
    };

    for (int i = 0; excluded_tags[i]; i++) {
        if (strcmp(tag_name, excluded_tags[i]) == 0)
            return 1;
    }

    return 0;
}

void convert_node(LaTeXConverter* converter, HTMLNode* node) {
    if (!node) return;

    /* handle text nodes - including those with only whitespace */
    if (!node->tag && node->content) {
        escape_latex(converter, node->content);
        return;
    }

    if (!node->tag) return;

    /* skip excluded elements and all their child elements completely */
    if (node->tag && should_exclude_tag(node->tag))
        return;

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
    /* image support */
    else if (strcmp(node->tag, "img") == 0) {
        converter->image_counter++;
        char* src = get_attribute(node->attributes, "src");

        char* alt = get_attribute(node->attributes, "alt");
        char* width_attr = get_attribute(node->attributes, "width");

        char* height_attr = get_attribute(node->attributes, "height");
        char* image_id_attr = get_attribute(node->attributes, "id");

        if (src) {
            char* image_path = NULL;

            /* download image if enabled and we have a directory */
            if (converter->download_images && converter->image_output_dir)
                image_path = download_image_src(src, converter->image_output_dir, converter->image_counter);

            /* if download failed or not enabled, use original src */
            if (!image_path) {
                /* force download for base64 images */
                if (is_base64_image(src) && converter->download_images && converter->image_output_dir)
                    image_path = download_image_src(src, converter->image_output_dir, converter->image_counter);

                /* use original source path */
                if (!image_path) image_path = strdup(src);
            }

            /* start figure environment */
            append_string(converter, "\n\n\\begin{figure}[h]\n");

            /* default centering */
            append_string(converter, "\\centering\n");

            /* parse CSS style for width, height overrides */
            int width_pt = 0;
            int height_pt = 0;

            /* check if CSS style overrides width/height */
            if (css_props) {
                if (css_props->width) width_pt = css_length_to_pt(css_props->width);
                if (css_props->height) height_pt = css_length_to_pt(css_props->height);
            }

            /* fall back to attribute values if CSS didn't provide dimensions */
            if (width_pt == 0 && width_attr) width_pt = css_length_to_pt(width_attr);
            if (height_pt == 0 && height_attr) height_pt = css_length_to_pt(height_attr);

            /* escape the image path for LaTeX */
            append_string(converter, "\\includegraphics");

            /* add width/height options if specified */
            if (width_pt > 0 || height_pt > 0) {
                append_string(converter, "[");

                if (width_pt > 0) {
                    char width_str[32];
                    snprintf(width_str, sizeof(width_str), "width=%dpt", width_pt);
                    append_string(converter, width_str);
                }

                if (height_pt > 0) {
                    if (width_pt > 0) append_string(converter, ",");
                    char height_str[32];

                    snprintf(height_str, sizeof(height_str), "height=%dpt", height_pt);
                    append_string(converter, height_str);
                }

                append_string(converter, "]");
            }

            append_string(converter, "{");

            /* use directory/filename format for downloaded images */
            if (converter->download_images && converter->image_output_dir 
                && strstr(image_path, converter->image_output_dir) == image_path)
                escape_latex_special(converter, image_path + 2);
            else
                /* use original path */
                escape_latex(converter, image_path);

            append_string(converter, "}\n");

            /* add caption if alt text is present */
            if (alt && alt[0] != '\0') {
                append_string(converter, "\n");
                append_string(converter, "\\caption{");

                escape_latex(converter, alt);
                append_string(converter, "}\n");
            }
            else {
                /* automatic caption generation using the image caption counter */
                converter->state.image_caption_counter++;
                append_string(converter, "\\caption{");
                char text_caption[16];

                char caption_counter[10];
                itoa(converter->state.image_caption_counter, caption_counter, 10);

                strcpy(text_caption, "Image ");
                strcpy(text_caption + 6, caption_counter);

                escape_latex(converter, text_caption);
                append_string(converter, "}\n");
            }

            /* add figure label if the image id attribute is present */
            if (image_id_attr && image_id_attr[0] != '\0') {
                append_string(converter, "\\label{fig:");

                escape_latex(converter, image_id_attr);
                append_string(converter, "}\n");
            }
            else {
                /* automatic ID generation using the image id counter */
                converter->state.image_id_counter++;
                append_string(converter, "\\label{fig:");
                char image_label_id[16];

                char label_counter[10];
                itoa(converter->state.image_id_counter, label_counter, 10);

                strcpy(image_label_id, "image_");
                strcpy(image_label_id + 6, label_counter);

                escape_latex_special(converter, image_label_id);
                append_string(converter, "}\n");
            }

            /* end figure environment */
            append_string(converter, "\\end{figure}\n");
            append_string(converter, "\\FloatBarrier\n\n");
        }
    }
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

        const char* table_id = get_attribute(node->attributes, "id");

        /* reset CSS state after table */
        if (table_id && table_id[0] != '\0')
            end_table(converter, table_id);
        else {
            converter->state.table_id_counter++;
            char table_label[16];

            char label_counter[10];
            itoa(converter->state.table_id_counter, label_counter, 10);

            strcpy(table_label, "table_");
            strcpy(table_label + 6, label_counter);
            end_table(converter, table_label);
        }

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