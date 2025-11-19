#include "html2tex.h"
#include <stdlib.h>
#include <string.h>

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

    converter->error_message[0] = '\0';
    return converter;
}

char* html2tex_convert(LaTeXConverter* converter, const char* html) {
    if (!converter || !html)
        return NULL;

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

    /* return a copy of the output */
    char* result = malloc(converter->output_size + 1);
    if (result) strcpy(result, converter->output);
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

    if (converter->output)
        free(converter->output);

    free(converter);
}