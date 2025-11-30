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
    if (state->position < state->length && state->input[state->position] == '/') {
        state->position++;
        char* tag_name = parse_tag_name(state);
        skip_whitespace(state);

        if (state->position < state->length && state->input[state->position] == '>')
            state->position++;

        /* closing tags do not create nodes */
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

    /* parse children if not self-closing and not a void element */
    if (!self_closing) {
        /* void elements in HTML cannot have content */
        const char* void_elements[] = {
            "area", "base", "br", "col", "embed", "hr", "img",
            "input", "link", "meta", "param", "source", "track", "wbr", NULL
        };

        int is_void_element = 0;
        for (int i = 0; void_elements[i]; i++) {
            if (strcmp(tag_name, void_elements[i]) == 0) {
                is_void_element = 1;
                break;
            }
        }

        if (!is_void_element) {
            HTMLNode** current_child = &node->children;

            while (state->position < state->length) {
                /* do not skip whitespace between children - it might be significant text */

                /* check for closing tag */
                if (state->position < state->length - 1 && state->input[state->position] == '<' && state->input[state->position + 1] == '/') {
                    /* found closing tag - skip whitespace before checking */
                    size_t saved_pos = state->position;
                    skip_whitespace(state);

                    /* if we still have the closing tag after skipping whitespace */
                    if (state->position < state->length - 1 && state->input[state->position] == '<' && state->input[state->position + 1] == '/') {
                        state->position += 2;
                        char* closing_tag = parse_tag_name(state);
                        skip_whitespace(state);

                        /* only break if this is the correct closing tag for our element */
                        if (closing_tag && tag_name && strcmp(closing_tag, tag_name) == 0) {
                            if (state->position < state->length && state->input[state->position] == '>') {
                                state->position++;
                                free(closing_tag);
                                break;
                            }
                        }

                        /* not our closing tag, restore position and continue parsing */
                        free(closing_tag);
                        state->position = saved_pos;
                    }
                    else
                        /* the whitespace was actually text content, restore position and parse as text */
                        state->position = saved_pos;
                }

                HTMLNode* child = parse_node(state);

                if (child) {
                    child->parent = node;
                    *current_child = child;
                    current_child = &child->next;
                }
                else
                    /* if no child was parsed, we might be at the end or have malformed HTML */
                    break;
            }
        }
    }

    return node;
}

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

HTMLNode* dom_tree_copy(HTMLNode* node) {
    if (!node) return NULL;

    HTMLNode* new_node = (HTMLNode*)malloc(sizeof(HTMLNode));
    if (!new_node) return NULL;

    new_node->tag = node->tag ? strdup(node->tag) : NULL;
    new_node->content = node->content ? strdup(node->content) : NULL;
    new_node->parent = NULL;

    new_node->next = NULL;
    new_node->children = NULL;

    /* copy attributes */
    HTMLAttribute* new_attrs = NULL;

    HTMLAttribute** current_attr = &new_attrs;
    HTMLAttribute* old_attr = node->attributes;

    while (old_attr) {
        HTMLAttribute* new_attr = (HTMLAttribute*)malloc(sizeof(HTMLAttribute));

        if (!new_attr) {
            html2tex_free_node(new_node);
            return NULL;
        }

        new_attr->key = strdup(old_attr->key);
        new_attr->value = old_attr->value ? strdup(old_attr->value) : NULL;

        new_attr->next = NULL;
        *current_attr = new_attr;

        current_attr = &new_attr->next;
        old_attr = old_attr->next;
    }

    /* recursively copy children */
    new_node->attributes = new_attrs;
    HTMLNode* new_children = NULL;

    HTMLNode** current_child = &new_children;
    HTMLNode* old_child = node->children;

    while (old_child) {
        HTMLNode* copied_child = dom_tree_copy(old_child);
        if (!copied_child) {
            html2tex_free_node(new_node);
            return NULL;
        }

        copied_child->parent = new_node;
        *current_child = copied_child;

        current_child = &copied_child->next;
        old_child = old_child->next;
    }

    new_node->children = new_children;
    return new_node;
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