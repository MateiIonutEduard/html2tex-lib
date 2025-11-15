#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 4096

typedef struct {
    const char* input;
    size_t position;
    size_t length;
} ParserState;

static void skip_whitespace(ParserState* state) {
    while (state->position < state->length &&
        isspace(state->input[state->position]))
        state->position++;
}

static char* parse_tag_name(ParserState* state) {
    size_t start = state->position;

    while (state->position < state->length &&
        (isalnum(state->input[state->position]) ||
            state->input[state->position] == '-'))
        state->position++;

    if (state->position == start) return NULL;
    size_t length = state->position - start;
    char* name = malloc(length + 1);

    strncpy(name, state->input + start, length);
    name[length] = '\0';

    /* convert to lowercase for consistency */
    for (size_t i = 0; i < length; i++)
        name[i] = tolower(name[i]);

    return name;
}