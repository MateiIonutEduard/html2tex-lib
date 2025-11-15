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

    converter->error_code = 0;
    converter->error_message[0] = '\0';
    return converter;
}