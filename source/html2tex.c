#include "html2tex.h"
#include <stdlib.h>
#include <string.h>

char* portable_strdup(const char* str) {
    if (!str) return NULL;

    size_t len = strlen(str) + 1;
    char* copy = malloc(len);

    if (copy)
        memcpy(copy, str, len);

    return copy;
}

LaTeXConverter* html2tex_create(void) {
    LaTeXConverter* converter = malloc(sizeof(LaTeXConverter));
    if (!converter) return NULL;

    converter->output = NULL;
    converter->output_size = 0;

    converter->output_capacity = 0;
    converter->state.indent_level = 0;

    converter->state.list_level = 0;
    converter->state.in_paragraph = 0;

    converter->state.in_list = 0;
    converter->state.table_counter = 0;

    converter->state.figure_counter = 0;
    converter->state.table_id_counter = 0;

    converter->state.image_id_counter = 0;
    converter->state.image_caption_counter = 0;
    converter->state.figure_id_counter = 0;

    converter->state.in_table = 0;
    converter->state.in_table_row = 0;

    converter->state.in_table_cell = 0;
    converter->state.table_columns = 0;

    converter->state.current_column = 0;
    converter->state.table_caption = NULL;

    /* initialize CSS state tracking */
    converter->state.css_braces = 0;
    converter->state.css_environments = 0;

    converter->state.pending_margin_bottom = 0;
    converter->state.has_bold = 0;

    converter->state.has_italic = 0;
    converter->state.has_underline = 0;

    converter->state.has_color = 0;
    converter->state.has_background = 0;

    converter->state.has_font_family = 0;
    converter->error_code = 0;

    /* initialize image configuration */
    converter->image_output_dir = NULL;
    converter->download_images = 0;
    converter->image_counter = 0;

    converter->error_message[0] = '\0';
    return converter;
}

LaTeXConverter* html2tex_copy(LaTeXConverter* converter) {
    if (!converter) return NULL;
    LaTeXConverter* clone = malloc(sizeof(LaTeXConverter));

    if (!clone) return NULL;
    clone->output = converter->output ? strdup(converter->output) : NULL;
    clone->output_size = converter->output_size;

    clone->output_capacity = converter->output_capacity;
    clone->state.indent_level = converter->state.indent_level;

    clone->state.list_level = converter->state.list_level;
    clone->state.in_paragraph = converter->state.in_paragraph;

    clone->state.in_list = converter->state.in_list;
    clone->state.table_counter = converter->state.table_counter;

    clone->state.figure_counter = converter->state.figure_counter;
    clone->state.table_id_counter = converter->state.table_id_counter;

    clone->state.image_id_counter = converter->state.image_id_counter;
    clone->state.image_caption_counter = converter->state.image_caption_counter;
    clone->state.figure_id_counter = converter->state.figure_id_counter;

    clone->state.in_table = converter->state.in_table;
    clone->state.in_table_row = converter->state.in_table_row;

    clone->state.in_table_cell = converter->state.in_table_cell;
    clone->state.table_columns = converter->state.table_columns;

    clone->state.current_column = converter->state.current_column;
    clone->state.table_caption = converter->state.table_caption ? 
        strdup(converter->state.table_caption) : NULL;

    /* copy of CSS state tracking */
    clone->state.css_braces = converter->state.css_braces;
    clone->state.css_environments = converter->state.css_environments;

    clone->state.pending_margin_bottom = converter->state.pending_margin_bottom;
    clone->state.has_bold = converter->state.has_bold;

    clone->state.has_italic = converter->state.has_italic;
    clone->state.has_underline = converter->state.has_underline;

    clone->state.has_color = converter->state.has_color;
    clone->state.has_background = converter->state.has_background;

    clone->state.has_font_family = converter->state.has_font_family;
    clone->error_code = converter->error_code;

    /* copy image configuration */
    clone->image_output_dir = converter->image_output_dir ? strdup(converter->image_output_dir) : NULL;
    clone->download_images = converter->download_images;
    clone->image_counter = converter->image_counter;

    /* copy error message safely */
    if (converter->error_message[0] != '\0') {
        strncpy(clone->error_message, converter->error_message, sizeof(clone->error_message) - 1);
        clone->error_message[sizeof(clone->error_message) - 1] = '\0';
    }
    else clone->error_message[0] = '\0';
    return clone;
}

void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir) {
    if (!converter) return;

    /* free existing directory if set */
    if (converter->image_output_dir) {
        free(converter->image_output_dir);
        converter->image_output_dir = NULL;
    }

    if (dir && dir[0] != '\0')
        converter->image_output_dir = strdup(dir);
}

void html2tex_set_download_images(LaTeXConverter* converter, int enable) {
    if (converter)
        converter->download_images = enable ? 1 : 0;
}

char* html2tex_convert(LaTeXConverter* converter, const char* html) {
    if (!converter || !html)
        return NULL;

    /* initialize image utilities if downloading is enabled */
    if (converter->download_images) image_utils_init();

    /* reset converter state */
    if (converter->output) {
        free(converter->output);
        converter->output = NULL;
    }

    converter->output_size = 0;
    converter->output_capacity = 0;

    converter->error_code = 0;
    converter->error_message[0] = '\0';

    /* free any existing caption */
    if (converter->state.table_caption) {
        free(converter->state.table_caption);
        converter->state.table_caption = NULL;
    }

    /* Reset CSS state */
    reset_css_state(converter);

    /* add LaTeX document preamble */
    append_string(converter, "\\documentclass{article}\n");
    append_string(converter, "\\usepackage{hyperref}\n");

    append_string(converter, "\\usepackage{ulem}\n");
    append_string(converter, "\\usepackage[table]{xcolor}\n");

    append_string(converter, "\\usepackage{tabularx}\n");
    append_string(converter, "\\usepackage{graphicx}\n");

    append_string(converter, "\\usepackage{placeins}\n");
    append_string(converter, "\\begin{document}\n\n");

    /* parse HTML and convert */
    HTMLNode* root = html2tex_parse(html);

    if (root) {
        convert_children(converter, root);
        html2tex_free_node(root);
    }
    else {
        converter->error_code = 1;
        strcpy(converter->error_message, "Failed to parse HTML");
        return NULL;
    }

    /* add document ending */
    append_string(converter, "\n\\end{document}\n");

    /* cleanup image utilities if they were initialized */
    if (converter->download_images) image_utils_cleanup();

    /* return a copy of the output */
    char* result = malloc(converter->output_size + 1);

    if (result) {
        if (converter->output && converter->output_size > 0) {
            memcpy(result, converter->output, converter->output_size);
            result[converter->output_size] = '\0';
        }
        else result[0] = '\0';
    }

    return result;
}

int html2tex_get_error(const LaTeXConverter* converter) {
    return converter ? converter->error_code : -1;
}

const char* html2tex_get_error_message(const LaTeXConverter* converter) {
    return converter ? converter->error_message : "Invalid converter";
}

void html2tex_destroy(LaTeXConverter* converter) {
    if (!converter) return;

    /* free table caption if it exists */
    if (converter->state.table_caption)
        free(converter->state.table_caption);

    /* free image directory */
    if (converter->image_output_dir)
        free(converter->image_output_dir);

    if (converter->output)
        free(converter->output);

    free(converter);
}