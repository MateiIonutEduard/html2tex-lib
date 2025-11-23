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

static char* parse_quoted_string(ParserState* state) {
    if (state->position >= state->length)
        return NULL;

    char quote = state->input[state->position];
    if (quote != '"' && quote != '\'') return NULL;

    /* skip opening quote */
    state->position++;

    size_t start = state->position;
    while (state->position < state->length &&
        state->input[state->position] != quote)
        state->position++;

    if (state->position >= state->length) return NULL;
    size_t length = state->position - start;

    char* str = malloc(length + 1);
    strncpy(str, state->input + start, length);

    /* skip closing quote */
    str[length] = '\0';
    state->position++;

    return str;
}

static HTMLAttribute* parse_attributes(ParserState* state) {
    HTMLAttribute* head = NULL;
    HTMLAttribute** current = &head;

    while (state->position < state->length) {
        skip_whitespace(state);

        if (state->input[state->position] == '>' ||
            state->input[state->position] == '/')
            break;

        char* key = parse_tag_name(state);
        if (!key) break;

        skip_whitespace(state);
        char* value = NULL;

        if (state->position < state->length && state->input[state->position] == '=') {
            state->position++;
            skip_whitespace(state);
            value = parse_quoted_string(state);
        }

        HTMLAttribute* attr = malloc(sizeof(HTMLAttribute));
        attr->key = key;

        attr->value = value;
        attr->next = NULL;

        *current = attr;
        current = &(*current)->next;
    }

    return head;
}

static char* parse_text_content(ParserState* state) {
    size_t start = state->position;
    size_t length = 0;

    while (state->position < state->length) {
        if (state->input[state->position] == '<')
            break;

        state->position++;
        length++;
    }

    if (length == 0) return NULL;

    char* text = malloc(length + 1);
    strncpy(text, state->input + start, length);
    text[length] = '\0';

    return text;
}

static HTMLNode* parse_element(ParserState* state);

static HTMLNode* parse_node(ParserState* state) {
    /* don't skip whitespace at the beginning - it might be significant */
    if (state->position >= state->length)
        return NULL;

    if (state->input[state->position] == '<')
        return parse_element(state);
    else {
        HTMLNode* node = malloc(sizeof(HTMLNode));
        node->tag = NULL;

        node->content = parse_text_content(state);
        node->attributes = NULL;

        node->children = NULL;
        node->next = NULL;

        node->parent = NULL;

        /* always return the node, even if content is only whitespace */
        /* this is crucial for preserving spaces between elements */
        return node;
    }
}

static HTMLNode* parse_element(ParserState* state) {
    if (state->input[state->position] != '<') return NULL;
    state->position++;

    /* check for closing tag */
    if (state->input[state->position] == '/') {
        state->position++;
        char* tag_name = parse_tag_name(state);
        skip_whitespace(state);

        if (state->position < state->length && state->input[state->position] == '>')
            state->position++;

        /* closing tags don't create nodes */
        free(tag_name);
        return NULL;
    }

    /* parse opening tag */
    char* tag_name = parse_tag_name(state);

    if (!tag_name) return NULL;
    HTMLAttribute* attributes = parse_attributes(state);

    /* check for self-closing tag */
    int self_closing = 0;

    if (state->position < state->length && state->input[state->position] == '/') {
        self_closing = 1;
        state->position++;
    }

    if (state->position < state->length && state->input[state->position] == '>')
        state->position++;

    HTMLNode* node = malloc(sizeof(HTMLNode));
    node->tag = tag_name;

    node->content = NULL;
    node->attributes = attributes;

    node->children = NULL;
    node->next = NULL;
    node->parent = NULL;

    /* parse children if not self-closing */
    if (!self_closing) {
        HTMLNode** current_child = &node->children;

        while (state->position < state->length) {
            /* don't skip whitespace between children - it might be significant text */

            /* check for closing tag */
            if (state->position < state->length - 1 &&
                state->input[state->position] == '<' &&
                state->input[state->position + 1] == '/') {
                /* found closing tag - skip whitespace before checking */
                size_t saved_pos = state->position;
                skip_whitespace(state);

                /* if we still have the closing tag after skipping whitespace */
                if (state->position < state->length - 1 &&
                    state->input[state->position] == '<' &&
                    state->input[state->position + 1] == '/') {
                    state->position += 2;

                    char* closing_tag = parse_tag_name(state);
                    skip_whitespace(state);

                    if (state->position < state->length && state->input[state->position] == '>')
                        state->position++;

                    free(closing_tag);
                    break;
                }
                else {
                    /* the whitespace was actually text content, restore position and parse as text */
                    state->position = saved_pos;
                }
            }

            HTMLNode* child = parse_node(state);

            if (child) {
                child->parent = node;
                *current_child = child;
                current_child = &child->next;
            }
            else {
                /* if no child was parsed, we might be at the end or have malformed HTML */
                break;
            }
        }
    }

    return node;
}

/* Parse the virtual DOM tree without optimizations. */
HTMLNode* html2tex_parse(const char* html) {
    if (!html) return NULL;
    ParserState state;

    state.input = html;
    state.position = 0;

    state.length = strlen(html);
    HTMLNode* root = malloc(sizeof(HTMLNode));

    root->tag = NULL;
    root->content = NULL;

    root->attributes = NULL;
    root->children = NULL;

    root->next = NULL;
    root->parent = NULL;

    HTMLNode** current = &root->children;

    while (state.position < state.length) {
        HTMLNode* node = parse_node(&state);

        if (node) {
            *current = node;
            current = &node->next;
        }
        else {
            /* skip one character if parsing fails to avoid infinite loop */
            if (state.position < state.length)
                state.position++;
            else
                break;
        }
    }

    return root;
}

/* Parse the virtual DOM tree with minification for improved performance. */
HTMLNode* html2tex_parse_minified(const char* html) {
    if (!html) return NULL;

    /* first parse normally */
    HTMLNode* parsed = html2tex_parse(html);
    if (!parsed) return NULL;

    /* then minify */
    HTMLNode* minified = html2tex_minify_html(parsed);

    html2tex_free_node(parsed);
    return minified;
}

void html2tex_free_node(HTMLNode* node) {
    if (!node) return;
    HTMLNode* current = node;

    while (current) {
        HTMLNode* next = current->next;
        if (current->tag) free(current->tag);

        if (current->content) free(current->content);
        HTMLAttribute* attr = current->attributes;

        while (attr) {
            HTMLAttribute* next_attr = attr->next;
            free(attr->key);

            if (attr->value)
                free(attr->value);

            free(attr);
            attr = next_attr;
        }

        html2tex_free_node(current->children);
        free(current);
        current = next;
    }
}