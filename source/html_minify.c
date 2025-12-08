#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "html2tex.h"

/* Check whether a tag is safe to minify by removing surrounding whitespace. */
static int is_safe_to_minify_tag(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    /* read-only tags where whitespace is significant */
    static const char* const preserve_whitespace_tags[] = {
        "pre", "code", "textarea", "script", "style", NULL
    };

    /* compute length once, because the most tags will fail this check */
    size_t len = 0;

    const char* p = tag_name;
    while (*p) { len++; p++; }

    /* quick length-based rejection */
    switch (len) {
    case 3: case 4: case 5: case 6: case 8:
        break;
    default:
        return 1;
    }

    /* check first character before strcmp */
    char first_char = tag_name[0];

    for (int i = 0; preserve_whitespace_tags[i]; i++) {
        /* fast rejection before expensive strcmp function call */
        if (preserve_whitespace_tags[i][0] != first_char) continue;

        /* apply strcmp function only for the few cases */
        if (strcmp(tag_name, preserve_whitespace_tags[i]) == 0)
            return 0;
    }

    return 1;
}

/* Check if text content is only whitespace. */
static int is_whitespace_only(const char* text) {
    if (!text) return 1;
    const unsigned char* p = (const unsigned char*)text;

    while (*p) {
        unsigned char c = *p++;
        switch (c) {
        case ' ': case '\t': case '\n': case '\v': case '\f': case '\r':
            continue;
        default:
            return 0;
        }
    }
}

/* Remove the unnecessary whitespace from text content. */
static char* minify_text_content(const char* text, int is_in_preformatted) {
    /* quick null check */
    if (!text) return NULL;

    /* preformatted content (copy as-is) */
    if (is_in_preformatted) return strdup(text);
    const unsigned char* src = (const unsigned char*)text;

    /* empty string */
    if (*src == '\0') return NULL;

    /* single character, common in HTML */
    if (src[1] == '\0') {
        /* check if it's whitespace */
        unsigned char c = src[0];

        if (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r')
            return NULL;

        /* non-whitespace single char found */
        char* result = (char*)malloc(2);

        if (result) {
            result[0] = c;
            result[1] = '\0';
        }

        return result;
    }

    /* analyze string and compute final size */
    const unsigned char* scan = src;
    size_t final_size = 0;
    int in_whitespace = 0;
    int has_content = 0;

    while (*scan) {
        unsigned char c = *scan;

        /* using ASCII whitespace check, that is faster than isspace */
        if (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r') {
            /* only count space if not consecutive and not at start */
            if (!in_whitespace && has_content)
                final_size++;
            
            in_whitespace = 1;
        }
        else {
            /* non-whitespace char */
            final_size++;
            has_content = 1;
            in_whitespace = 0;
        }
        scan++;
    }

    /* all whitespace or empty result */
    if (final_size == 0) return NULL;

    /* adjust for trailing whitespace */
    if (in_whitespace && final_size > 0)
        final_size--;

    /* no whitespace at all */
    if (final_size == (size_t)(scan - src)) {
        /* just copy the string */
        char* result = (char*)malloc(final_size + 1);
        if (!result) return NULL;

        memcpy(result, text, final_size);
        result[final_size] = '\0';
        return result;
    }

    /* alloc exact size needed */
    char* result = (char*)malloc(final_size + 1);
    if (!result) return NULL;

    /* build minified string */
    char* dest = result;
    in_whitespace = 0;
    int at_start = 1;

    while (*src) {
        unsigned char c = *src++;

        if (c == ' ' || c == '\t' || c == '\n' ||
            c == '\v' || c == '\f' || c == '\r') {
            /* collapse multiple whitespace to single space */
            if (!in_whitespace && !at_start)
                *dest++ = ' ';

            in_whitespace = 1;
        }
        else {
            /* copy non-whitespace char */
            *dest++ = c;
            in_whitespace = 0;
            at_start = 0;
        }
    }

    /* remove trailing space if we added one */
    if (in_whitespace && dest > result)
        dest--;

    *dest = '\0';
    return result;
}

/* Minify the attribute value by removing unnecessary quotes when possible. */
static char* minify_attribute_value(const char* value) {
    if (!value) return NULL;
    const unsigned char* src = (const unsigned char*)value;

    /* empty string */
    if (*src == '\0') {
        char* r = (char*)malloc(3);
        if (r) { r[0] = '"'; r[1] = '"'; r[2] = '\0'; }
        return r;
    }

    /* single pass to analyze and optionally build */
    int needs_quotes = 0;
    int has_single = 0;
    int has_double = 0;
    size_t double_count = 0;
    size_t len = 0;

    /* quick check for simple strings */
    while (*src) {
        unsigned char c = *src++;
        len++;

        /* check for char requiring quotes */
        switch (c) {
        case ' ': case '\t': case '\n': case '\r':
        case '\f': case '\v': case '=': case '<':
        case '>': case '`':
            needs_quotes = 1;
            break;
        case '\'':
            has_single = 1;
            break;
        case '"':
            has_double = 1;
            double_count++;
            break;
        }

        /* if need quotes and have both quote types, break early */
        if (needs_quotes && has_single && has_double) break;
    }

    /* reset pointer for potential second pass */
    src = (const unsigned char*)value;

    /* no quotes needed */
    if (!needs_quotes) {
        char* result = (char*)malloc(len + 1);

        if (result) {
            memcpy(result, value, len);
            result[len] = '\0';
        }

        return result;
    }

    /* determine best quote type and build result */
    if (!has_double) {
        /* use double quotes, no escape needed */
        char* result = (char*)malloc(len + 3);
        if (!result) return NULL;

        result[0] = '"';
        memcpy(result + 1, value, len);
        result[len + 1] = '"';
        result[len + 2] = '\0';
        return result;
    }

    if (!has_single) {
        /* use single quotes without escape */
        char* result = (char*)malloc(len + 3);
        if (!result) return NULL;

        result[0] = '\'';
        memcpy(result + 1, value, len);
        result[len + 1] = '\'';
        result[len + 2] = '\0';
        return result;
    }

    /* need to escape double quotes */
    size_t total_len = len + double_count + 3;

    char* result = (char*)malloc(total_len);
    if (!result) return NULL;

    char* dest = result;
    *dest++ = '"';

    while (*src) {
        if (*src == '"') *dest++ = '\\';
        *dest++ = *src++;
    }

    *dest++ = '"';
    *dest = '\0';

    return result;
}

/* Fast minification function of HTML's DOM tree. */
static HTMLNode* minify_node(HTMLNode* node, int in_preformatted) {
    if (!node) return NULL;

    /* precompute essential tag lookup tables */
    static const char* const preserve_tags[] = { "pre", "code", "textarea", "script", "style", NULL };
    static const char* const void_tags[] = { "area", "base", "br", "col", "embed", "hr", "img",
        "input", "link", "meta", "param", "source", "track", "wbr", NULL };
    static const char* const essential_tags[] = { "br", "hr", "img", "input", "meta", "link", NULL };

    /* create root node */
    HTMLNode* new_root = (HTMLNode*)calloc(1, sizeof(HTMLNode));
    if (!new_root) return NULL;

    /* copy root data with error checking */
    if (node->tag) {
        new_root->tag = strdup(node->tag);

        if (!new_root->tag) {
            free(new_root);
            return NULL;
        }
    }

    /* minify root attributes efficiently */
    if (node->attributes) {
        HTMLAttribute* new_attrs = NULL;
        HTMLAttribute** tail = &new_attrs;
        HTMLAttribute* src_attr = node->attributes;

        while (src_attr) {
            HTMLAttribute* new_attr = (HTMLAttribute*)malloc(sizeof(HTMLAttribute));
            if (!new_attr) goto cleanup_root;
            new_attr->key = strdup(src_attr->key);

            if (!new_attr->key) {
                free(new_attr);
                goto cleanup_root;
            }

            new_attr->value = src_attr->value ?
                minify_attribute_value(src_attr->value) : NULL;
            new_attr->next = NULL;

            *tail = new_attr;
            tail = &new_attr->next;
            src_attr = src_attr->next;
        }

        new_root->attributes = new_attrs;
    }

    /* check if root is preformatted */
    int root_is_preformatted = in_preformatted;

    if (node->tag) {
        char first_char = node->tag[0];

        if (first_char == 'p' || first_char == 'c' || first_char == 't' || first_char == 's') {
            for (int i = 0; preserve_tags[i]; i++) {
                if (strcmp(node->tag, preserve_tags[i]) == 0) {
                    root_is_preformatted = 1;
                    break;
                }
            }
        }
    }

    /* minify root content */
    if (node->content) {
        if (is_whitespace_only(node->content) && !root_is_preformatted)
            new_root->content = NULL;
        else {
            new_root->content = minify_text_content(node->content, root_is_preformatted);
            if (!new_root->content && node->content) goto cleanup_root;
        }
    }

    /* check if root is void element */
    if (node->tag) {
        for (int i = 0; void_tags[i]; i++) {
            if (strcmp(node->tag, void_tags[i]) == 0)
                return new_root;
        }
    }

    NodeQueue* src_queue_front = NULL;
    NodeQueue* src_queue_rear = NULL;

    NodeQueue* dst_queue_front = NULL;
    NodeQueue* dst_queue_rear = NULL;

    NodeQueue* preformatted_queue_front = NULL;
    NodeQueue* preformatted_queue_rear = NULL;

    /* enqueue root for processing */
    if (!queue_enqueue(&src_queue_front, &src_queue_rear, node) ||
        !queue_enqueue(&dst_queue_front, &dst_queue_rear, new_root) ||
        !queue_enqueue(&preformatted_queue_front, &preformatted_queue_rear,
            (HTMLNode*)(intptr_t)root_is_preformatted))
        goto cleanup_all;

    /* BFS processing */
    while (src_queue_front) {
        HTMLNode* src_current = queue_dequeue(&src_queue_front, &src_queue_rear);
        HTMLNode* dst_current = queue_dequeue(&dst_queue_front, &dst_queue_rear);

        int current_preformatted = (int)(intptr_t)queue_dequeue(&preformatted_queue_front,
            &preformatted_queue_rear);

        /* determine if current node is safe to minify */
        if (!src_current || !dst_current) continue;
        int current_safe_to_minify = 1;

        if (src_current->tag) {
            for (int i = 0; preserve_tags[i]; i++) {
                if (strcmp(src_current->tag, preserve_tags[i]) == 0) {
                    current_safe_to_minify = 0;
                    break;
                }
            }
        }

        /* process children with tail pointer optimization */
        HTMLNode* src_child = src_current->children;
        HTMLNode** dst_child_tail = &dst_current->children;

        while (src_child) {
            /* skip whitespace before block elements if safe */
            HTMLNode* next_src_child = src_child->next;

            if (current_safe_to_minify && !current_preformatted) {
                if (src_child->tag && is_block_element(src_child->tag)) {
                    if (next_src_child && !next_src_child->tag &&
                        is_whitespace_only(next_src_child->content)) {
                        src_child = next_src_child;
                        next_src_child = src_child->next;
                    }
                }
            }

            /* create child node */
            HTMLNode* new_child = (HTMLNode*)calloc(1, sizeof(HTMLNode));
            if (!new_child) goto cleanup_all;

            /* copy tag */
            if (src_child->tag) {
                new_child->tag = strdup(src_child->tag);

                if (!new_child->tag) {
                    free(new_child);
                    goto cleanup_all;
                }
            }

            /* minify attributes */
            if (src_child->attributes) {
                HTMLAttribute* child_attrs = NULL;
                HTMLAttribute** attr_tail = &child_attrs;
                HTMLAttribute* src_attr = src_child->attributes;

                while (src_attr) {
                    HTMLAttribute* new_attr = (HTMLAttribute*)malloc(sizeof(HTMLAttribute));

                    if (!new_attr) {
                        free(new_child->tag);
                        free(new_child);
                        goto cleanup_all;
                    }

                    new_attr->key = strdup(src_attr->key);

                    if (!new_attr->key) {
                        free(new_attr);
                        free(new_child->tag);

                        free(new_child);
                        goto cleanup_all;
                    }

                    new_attr->value = src_attr->value ?
                        minify_attribute_value(src_attr->value) : NULL;
                    new_attr->next = NULL;

                    *attr_tail = new_attr;
                    attr_tail = &new_attr->next;
                    src_attr = src_attr->next;
                }
                new_child->attributes = child_attrs;
            }

            /* determine child's preformatted status */
            int child_preformatted = current_preformatted;

            if (src_child->tag) {
                for (int i = 0; preserve_tags[i]; i++) {
                    if (strcmp(src_child->tag, preserve_tags[i]) == 0) {
                        child_preformatted = 1;
                        break;
                    }
                }
            }

            /* minify content */
            if (src_child->content) {
                if (is_whitespace_only(src_child->content) && !child_preformatted) {
                    new_child->content = NULL;
                }
                else {
                    new_child->content = minify_text_content(src_child->content, child_preformatted);
                    if (!new_child->content && src_child->content) {
                        free(new_child->tag);
                        free(new_child);
                        goto cleanup_all;
                    }
                }
            }

            /* link child to parent with tail pointer */
            *dst_child_tail = new_child;
            dst_child_tail = &new_child->next;
            new_child->parent = dst_current;

            /* check if child is void element */
            int child_is_void = 0;

            if (src_child->tag) {
                for (int i = 0; void_tags[i]; i++) {
                    if (strcmp(src_child->tag, void_tags[i]) == 0) {
                        child_is_void = 1;
                        break;
                    }
                }
            }

            /* enqueue child for processing if it has children and is not void */
            if (!child_is_void && src_child->children) {
                if (!queue_enqueue(&src_queue_front, &src_queue_rear, src_child) ||
                    !queue_enqueue(&dst_queue_front, &dst_queue_rear, new_child) ||
                    !queue_enqueue(&preformatted_queue_front, &preformatted_queue_rear,
                        (HTMLNode*)(intptr_t)child_preformatted)) {
                    free(new_child->tag);
                    free(new_child);
                    goto cleanup_all;
                }
            }

            /* remove empty non-essential nodes immediately */
            if (new_child->tag && !child_is_void && !new_child->children && !new_child->content) {
                int is_essential = 0;

                for (int i = 0; essential_tags[i]; i++) {
                    if (strcmp(new_child->tag, essential_tags[i]) == 0) {
                        is_essential = 1;
                        break;
                    }
                }

                if (!is_essential) {
                    /* remove from parent's list */
                    HTMLNode* prev = dst_current->children;

                    if (prev == new_child) {
                        dst_current->children = new_child->next;
                        dst_child_tail = &dst_current->children;
                    }
                    else {
                        while (prev && prev->next != new_child)
                            prev = prev->next;
                        if (prev) {
                            prev->next = new_child->next;

                            if (!new_child->next)
                                dst_child_tail = &prev->next;
                        }
                    }

                    html2tex_free_node(new_child);
                }
            }

            src_child = next_src_child;
        }
    }

    /* cleanup queues */
    queue_cleanup(&src_queue_front, &src_queue_rear);
    queue_cleanup(&dst_queue_front, &dst_queue_rear);
    queue_cleanup(&preformatted_queue_front, &preformatted_queue_rear);

    /* final check for empty non-essential root */
    if (new_root->tag && !new_root->children && !new_root->content) {
        int is_essential = 0;

        for (int i = 0; essential_tags[i]; i++) {
            if (strcmp(new_root->tag, essential_tags[i]) == 0) {
                is_essential = 1;
                break;
            }
        }

        if (!is_essential) {
            html2tex_free_node(new_root);
            return NULL;
        }
    }

    return new_root;

cleanup_root:
    html2tex_free_node(new_root);
    return NULL;

cleanup_all:
    /* cleanup everything */
    queue_cleanup(&src_queue_front, &src_queue_rear);
    queue_cleanup(&dst_queue_front, &dst_queue_rear);
    queue_cleanup(&preformatted_queue_front, &preformatted_queue_rear);
    html2tex_free_node(new_root);
    return NULL;
}

HTMLNode* html2tex_minify_html(HTMLNode* root) {
    if (!root) return NULL;

    /* alloc and zero-initialize in one call */
    HTMLNode* minified_root = (HTMLNode*)calloc(1, sizeof(HTMLNode));
    if (!minified_root) return NULL;

    /* process children iteratively with error handling */
    HTMLNode* src_child = root->children;
    HTMLNode** dst_tail = &minified_root->children;

    while (src_child) {
        /* minify child node */
        HTMLNode* minified_child = minify_node(src_child, 0);

        /* check for minification failure */
        if (!minified_child) {
            /* cleanup allocated memory before returning */
            html2tex_free_node(minified_root);
            return NULL;
        }

        /* link child to parent */
        minified_child->parent = minified_root;
        *dst_tail = minified_child;

        dst_tail = &minified_child->next;
        src_child = src_child->next;
    }

    return minified_root;
}