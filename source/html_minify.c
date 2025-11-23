#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "html2tex.h"

/* Check whether a tag is safe to minify by removing surrounding whitespace. */
static int is_safe_to_minify_tag(const char* tag_name) {
    if (!tag_name) return 0;

    /* tags where whitespace is significant */
    const char* preserve_whitespace_tags[] = {
        "pre", "code", "textarea", "script", "style", NULL
    };

    for (int i = 0; preserve_whitespace_tags[i]; i++) {
        if (strcmp(tag_name, preserve_whitespace_tags[i]) == 0)
            return 0;
    }

    return 1;
}

/* Check if text content is only whitespace. */
static int is_whitespace_only(const char* text) {
    if (!text) return 1;

    for (const char* p = text; *p; p++) {
        if (!isspace(*p))
            return 0;
    }

    return 1;
}

/* Remove the unnecessary whitespace from text content. */
static char* minify_text_content(const char* text, int is_in_preformatted) {
    if (!text || is_in_preformatted)
        return text ? strdup(text) : NULL;

    /* for normal text, collapse multiple whitespace into single space */
    char* result = malloc(strlen(text) + 1);
    if (!result) return NULL;

    char* dest = result;
    int in_whitespace = 0;

    for (const char* src = text; *src; src++) {
        if (isspace(*src)) {
            if (!in_whitespace && dest != result) {
                *dest++ = ' ';
                in_whitespace = 1;
            }
        }
        else {
            *dest++ = *src;
            in_whitespace = 0;
        }
    }

    *dest = '\0';

    /* trim leading/trailing whitespace */
    if (dest > result && isspace(*(dest - 1)))
        *(dest - 1) = '\0';

    /* if result is empty after trimming, return NULL */
    if (result[0] == '\0') {
        free(result);
        return NULL;
    }

    return result;
}

/* Minify the attribute value by removing unnecessary quotes when possible. */
static char* minify_attribute_value(const char* value) {
    if (!value) return NULL;

    /* if value is simple (no spaces, no special chars), we can remove quotes */
    int needs_quotes = 0;

    int has_single_quote = 0;
    int has_double_quote = 0;

    for (const char* p = value; *p; p++) {
        if (isspace(*p) || *p == '=' || *p == '<' || *p == '>' || *p == '`')
            needs_quotes = 1;
        
        if (*p == '\'') has_single_quote = 1;
        if (*p == '"') has_double_quote = 1;
    }

    /* empty value needs quotes */
    if (value[0] == '\0') needs_quotes = 1;
    if (!needs_quotes) return strdup(value);

    /* choose quote type that doesn't require escaping */
    char* result;

    if (!has_double_quote) {
        /* use double quotes */
        result = malloc(strlen(value) + 3);
        if (result) sprintf(result, "\"%s\"", value);
    }
    else if (!has_single_quote) {
        /* use single quotes */
        result = malloc(strlen(value) + 3);
        if (result) sprintf(result, "'%s'", value);
    }
    else {
        /* need to escape - use double quotes and escape existing doubles */
        size_t len = strlen(value);
        size_t new_len = len + 3;

        for (const char* p = value; *p; p++)
            if (*p == '"') new_len++;

        result = malloc(new_len);

        if (result) {
            char* dest = result;
            *dest++ = '"';

            for (const char* p = value; *p; p++) {
                if (*p == '"') *dest++ = '\\';
                *dest++ = *p;
            }

            *dest++ = '"';
            *dest = '\0';
        }
    }

    return result;
}

/* Recursive minification function */
static HTMLNode* minify_node_recursive(HTMLNode* node, int in_preformatted) {
    if (!node) return NULL;

    HTMLNode* new_node = malloc(sizeof(HTMLNode));
    if (!new_node) return NULL;

    /* copy the DOM structure */
    new_node->tag = node->tag ? strdup(node->tag) : NULL;
    new_node->parent = NULL;

    new_node->next = NULL;
    new_node->children = NULL;

    /* handle preformatted context */
    int current_preformatted = in_preformatted;

    if (node->tag) {
        if (strcmp(node->tag, "pre") == 0 || strcmp(node->tag, "code") == 0 ||
            strcmp(node->tag, "textarea") == 0 || strcmp(node->tag, "script") == 0 ||
            strcmp(node->tag, "style") == 0)
            current_preformatted = 1;
    }

    /* minify attributes */
    HTMLAttribute* new_attrs = NULL;

    HTMLAttribute** current_attr = &new_attrs;
    HTMLAttribute* old_attr = node->attributes;

    while (old_attr) {
        HTMLAttribute* new_attr = malloc(sizeof(HTMLAttribute));

        if (!new_attr) {
            html2tex_free_node(new_node);
            return NULL;
        }

        new_attr->key = strdup(old_attr->key);
        if (old_attr->value) new_attr->value = minify_attribute_value(old_attr->value);
        else new_attr->value = NULL;
        
        new_attr->next = NULL;
        *current_attr = new_attr;

        current_attr = &new_attr->next;
        old_attr = old_attr->next;
    }

    new_node->attributes = new_attrs;

    /* minify content */
    if (node->content) {
        if (is_whitespace_only(node->content) && !current_preformatted)
            /* remove whitespace-only text nodes outside preformatted blocks */
            new_node->content = NULL;
        else
            new_node->content = minify_text_content(node->content, current_preformatted);
    }
    else new_node->content = NULL;

    /* recursively minify children */
    HTMLNode* new_children = NULL;

    HTMLNode** current_child = &new_children;
    HTMLNode* old_child = node->children;

    int safe_to_minify = node->tag ? 
        is_safe_to_minify_tag(node->tag) : 1;

    while (old_child) {
        HTMLNode* minified_child = minify_node_recursive(old_child, current_preformatted);

        if (minified_child) {
            /* remove empty text nodes between elements (except in preformatted) */
            if (!minified_child->tag && !minified_child->content)
                html2tex_free_node(minified_child);
            else {
                /* remove whitespace between block elements */
                if (safe_to_minify && !current_preformatted) {
                    if (minified_child->tag && is_block_element(minified_child->tag)) {
                        /* skip whitespace before block elements */
                        HTMLNode* next = old_child->next;

                        /* skip the whitespace node */
                        if (next && !next->tag && is_whitespace_only(next->content))
                            old_child = next;
                    }
                }

                minified_child->parent = new_node;
                *current_child = minified_child;
                current_child = &minified_child->next;
            }
        }

        old_child = old_child->next;
    }

    new_node->children = new_children;

    /* remove empty nodes (except essential ones) */
    if (new_node->tag && !new_node->children && !new_node->content) {
        const char* essential_tags[] = {
            "br", "hr", "img", "input", "meta", "link", NULL
        };

        int is_essential = 0;

        for (int i = 0; essential_tags[i]; i++) {
            if (strcmp(new_node->tag, essential_tags[i]) == 0) {
                is_essential = 1;
                break;
            }
        }

        if (!is_essential) {
            html2tex_free_node(new_node);
            return NULL;
        }
    }

    return new_node;
}

/* Return the DOM tree after minification. */
HTMLNode* html2tex_minify_html(HTMLNode* root) {
    if (!root) return NULL;

    /* create a new minified tree */
    HTMLNode* minified_root = malloc(sizeof(HTMLNode));

    if (!minified_root) return NULL;
    minified_root->tag = NULL;

    minified_root->content = NULL;
    minified_root->attributes = NULL;

    minified_root->parent = NULL;
    minified_root->next = NULL;

    /* minify children */
    HTMLNode* new_children = NULL;

    HTMLNode** current_child = &new_children;
    HTMLNode* old_child = root->children;

    while (old_child) {
        HTMLNode* minified_child = minify_node_recursive(old_child, 0);

        if (minified_child) {
            minified_child->parent = minified_root;
            *current_child = minified_child;
            current_child = &minified_child->next;
        }

        old_child = old_child->next;
    }

    minified_root->children = new_children;
    return minified_root;
}