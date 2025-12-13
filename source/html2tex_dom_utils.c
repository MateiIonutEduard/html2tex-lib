#include "html2tex.h"

const char* get_attribute(HTMLAttribute* attrs, const char* key) {
    if (!key || key[0] == '\0') return NULL;
    size_t key_len = 0;

    /* precompute key length once for fast rejection */
    while (key[key_len]) key_len++;

    for (HTMLAttribute* attr = attrs; attr; attr = attr->next) {
        /* check first char for early rejection */
        if (!attr->key || attr->key[0] != key[0]) continue;

        /* length-based fast rejection */
        size_t attr_len = 0;

        while (attr->key[attr_len]) attr_len++;
        if (attr_len != key_len) continue;

        /* exact case-insensitive comparison */
        int match = 1;

        for (size_t i = 0; i < key_len; i++) {
            char c1 = attr->key[i];
            char c2 = key[i];

            /* fast ASCII case conversion */
            if ((c1 ^ c2) & 0x20) {
                /* convert both to lowercase for comparison */
                c1 |= 0x20;
                c2 |= 0x20;
            }

            if (c1 != c2) {
                match = 0;
                break;
            }
        }

        if (match) return attr->value;
    }

    return NULL;
}

int should_skip_nested_table(HTMLNode* node) {
    if (!node) return -1;
    NodeQueue* front = NULL;

    NodeQueue* rear = NULL;
    int result = 0;

    /* if current node is a table, check for nested tables in descendants */
    if (node->tag && strcmp(node->tag, "table") == 0) {
        /* if no children, definitely no nested tables */
        if (!node->children) return 0;

        /* enqueue direct children */
        HTMLNode* child = node->children;

        while (child) {
            if (!queue_enqueue(&front, &rear, child)) goto cleanup;
            child = child->next;
        }

        /* BFS for nested tables */
        while ((child = queue_dequeue(&front, &rear))) {
            if (child->tag && strcmp(child->tag, "table") == 0) {
                result = 1;
                goto cleanup;
            }

            /* enqueue children for further search */
            HTMLNode* grandchild = child->children;

            while (grandchild) {
                if (!queue_enqueue(&front, &rear, grandchild)) goto cleanup;
                grandchild = grandchild->next;
            }
        }
    }

    /* check parent hierarchy for table with nested tables */
    for (HTMLNode* parent = node->parent; parent; parent = parent->parent) {
        if (parent->tag && strcmp(parent->tag, "table") == 0) {
            /* clean and reuse the queue */
            queue_cleanup(&front, &rear);

            /* enqueue parent's children */
            HTMLNode* sibling = parent->children;

            while (sibling) {
                if (sibling != node && !queue_enqueue(&front, &rear, sibling)) goto cleanup;
                sibling = sibling->next;
            }

            /* BFS for nested tables in parent's descendants */
            HTMLNode* current;
            while ((current = queue_dequeue(&front, &rear))) {
                if (current->tag && strcmp(current->tag, "table") == 0) {
                    result = 1;
                    goto cleanup;
                }

                /* enqueue children for further search */
                HTMLNode* grandchild = current->children;
                while (grandchild) {
                    if (!queue_enqueue(&front, &rear, grandchild)) goto cleanup;
                    grandchild = grandchild->next;
                }
            }

            /* found nested table in parent chain? */
            if (result) break;
        }
    }

cleanup:
    queue_cleanup(&front, &rear);
    return result;
}

int table_contains_only_images(HTMLNode* node) {
    if (!node || !node->tag || strcmp(node->tag, "table") != 0)
        return 0;

    NodeQueue* front = NULL;
    NodeQueue* rear = NULL;
    int has_images = 0;

    /* enqueue direct children */
    for (HTMLNode* child = node->children; child; child = child->next)
        if (!queue_enqueue(&front, &rear, child)) goto cleanup;

    /* BFS traversal */
    HTMLNode* current;

    while ((current = queue_dequeue(&front, &rear))) {
        if (current->tag) {
            /* check for image tag */
            if (strcmp(current->tag, "img") == 0) {
                has_images = 1;
                continue;
            }

            /* check for structural table elements */
            if (strcmp(current->tag, "tbody") == 0 ||
                strcmp(current->tag, "thead") == 0 ||
                strcmp(current->tag, "tfoot") == 0 ||
                strcmp(current->tag, "tr") == 0 ||
                strcmp(current->tag, "td") == 0 ||
                strcmp(current->tag, "th") == 0 ||
                strcmp(current->tag, "caption") == 0) {

                /* enqueue children */
                for (HTMLNode* child = current->children; child; child = child->next)
                    if (!queue_enqueue(&front, &rear, child)) goto cleanup;

                continue;
            }

            /* any other tag means failure */
            has_images = 0;
            goto cleanup;
        }
        else if (current->content) {
            /* check for non-whitespace text */
            const char* p = current->content;

            while (*p) {
                if (!isspace(*p++)) {
                    has_images = 0;
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    queue_cleanup(&front, &rear);
    return has_images;
}

void convert_image_table(LaTeXConverter* converter, HTMLNode* node) {
    /* write figure header */
    append_string(converter, "\\begin{figure}[htbp]\n\\centering\n");
    append_string(converter, "\\setlength{\\fboxsep}{0pt}\n\\setlength{\\tabcolsep}{1pt}\n");

    /* start tabular */
    int columns = count_table_columns(node);
    append_string(converter, "\\begin{tabular}{");

    for (int i = 0; i < columns; i++) append_string(converter, "c");
    append_string(converter, "}\n");

    /* BFS for table rows */
    NodeQueue* queue = NULL, * rear = NULL;
    NodeQueue* cell_queue = NULL, * cell_rear = NULL;
    int first_row = 1;

    /* enqueue table children */
    for (HTMLNode* child = node->children; child; child = child->next) {
        queue_enqueue(&queue, &rear, child);
    }

    /* process table */
    HTMLNode* current;

    while ((current = queue_dequeue(&queue, &rear))) {
        if (!current->tag) continue;

        if (strcmp(current->tag, "tr") == 0) {
            if (!first_row) append_string(converter, " \\\\\n");
            first_row = 0;

            /* process cells in row */
            HTMLNode* cell = current->children;
            int col_count = 0;

            while (cell) {
                if (cell->tag && (strcmp(cell->tag, "td") == 0 || strcmp(cell->tag, "th") == 0)) {
                    if (col_count++ > 0) append_string(converter, " & ");

                    /* BFS search for image in cell */
                    cell_queue = cell_rear = NULL;

                    for (HTMLNode* cell_child = cell->children; cell_child; cell_child = cell_child->next)
                        queue_enqueue(&cell_queue, &cell_rear, cell_child);

                    int img_found = 0;
                    HTMLNode* cell_node;

                    while ((cell_node = queue_dequeue(&cell_queue, &cell_rear)) && !img_found) {
                        if (cell_node->tag && strcmp(cell_node->tag, "img") == 0) {
                            process_table_image(converter, cell_node);
                            img_found = 1;
                        }
                        else if (cell_node->tag) {
                            /* enqueue children for deeper search */
                            for (HTMLNode* grandchild = cell_node->children; grandchild; grandchild = grandchild->next)
                                queue_enqueue(&cell_queue, &cell_rear, grandchild);
                        }
                    }

                    if (!img_found) append_string(converter, " ");
                    queue_cleanup(&cell_queue, &cell_rear);
                }

                cell = cell->next;
            }
        }
        else if (strcmp(current->tag, "tbody") == 0 || strcmp(current->tag, "thead") == 0 ||
            strcmp(current->tag, "tfoot") == 0) {
            /* enqueue section children */
            for (HTMLNode* section_child = current->children; section_child; section_child = section_child->next)
                queue_enqueue(&queue, &rear, section_child);
        }
    }

    /* cleanup queues */
    queue_cleanup(&queue, &rear);
    queue_cleanup(&cell_queue, &cell_rear);

    /* finish tabular and figure */
    append_string(converter, "\n\\end{tabular}\n");

    append_figure_caption(converter, node);
    append_string(converter, "\\end{figure}\n\\FloatBarrier\n\n");
}

int is_block_element(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    static const struct {
        const char* tag;
        unsigned char first_char;
        const unsigned char length;
    } block_tags[] = {
        {"div", 'd', 3}, {"p", 'p', 1},
        {"h1", 'h', 2}, {"h2", 'h', 2}, {"h3", 'h', 2},
        {"h4", 'h', 2}, {"h5", 'h', 2}, {"h6", 'h', 2},
        {"ul", 'u', 2}, {"ol", 'o', 2}, {"li", 'l', 2},
        {"table", 't', 5}, {"tr", 't', 2}, {"td", 't', 2}, 
        {"th", 't', 2}, {"blockquote", 'b', 10}, {"section", 's', 7}, 
        {"article", 'a', 7}, {"header", 'h', 6}, {"footer", 'f', 6},
        {"nav", 'n', 3}, {"aside", 'a', 5}, {"main", 'm', 4}, 
        {"figure", 'f', 6}, {"figcaption", 'f', 10}, {NULL, 0, 0}
    };

    /* length-based tags detection */
    size_t len = 0;

    const char* p = tag_name;
    while (*p) {
        len++; p++;

        /* tag name is longer then expected, reject it */
        if (len > 10) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 1:  case 2:  case 3:  case 4:
    case 5: case 6:  case 7:  case 10:
        break;
    default:
        /* length does not match any block tag */
        return 0;
    }

    /* extract first char once */
    const unsigned char first_char = (unsigned char)tag_name[0];

    for (int i = 0; block_tags[i].tag; i++) {
        /* fast reject: first character mismatch */
        if (first_char != (unsigned char)block_tags[i].first_char) continue;

        /* medium reject: length mismatch, cheaper than strcmp */
        if (block_tags[i].length != len) continue;

        /* final verification: exact string match */
        if (strcmp(tag_name, block_tags[i].tag) == 0)
            return 1;
    }

    return 0;
}

int is_inline_element(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;

    static const struct {
        const char* tag;
        unsigned char first_char;
        const unsigned char length;
    } inline_tags[] = {
        {"a", 'a', 1}, {"abbr", 'a', 4}, {"b", 'b', 1},
        {"bdi", 'b', 3}, {"bdo", 'b', 3}, {"cite", 'c', 4},
        {"code", 'c', 4}, {"data", 'd', 4}, {"dfn", 'd', 3},
        {"em", 'e', 2}, {"font", 'f', 4}, {"i", 'i', 1},
        {"kbd", 'k', 3}, {"mark", 'm', 4}, {"q", 'q', 1},
        {"rp", 'r', 2}, {"rt", 'r', 2}, {"ruby", 'r', 4},
        {"samp", 's', 4}, {"small", 's', 5}, {"span", 's', 4},
        {"strong", 's', 6}, {"sub", 's', 3}, {"sup", 's', 3},
        {"time", 't', 4}, {"u", 'u', 1}, {"var", 'v', 3},
        {"wbr", 'w', 3}, {"br", 'b', 2}, {"img", 'i', 3},
        {"map", 'm', 3}, {"object", 'o', 6}, {"button", 'b', 6},
        {"input", 'i', 5}, {"label", 'l', 5}, {"meter", 'm', 5},
        {"output", 'o', 6}, {"progress", 'p', 8}, {"select", 's', 6},
        {"textarea", 't', 8}, {NULL, 0, 0}
    };

    /* compute the length with early bounds check */
    size_t len = 0;
    const char* p = tag_name;

    while (*p) {
        len++;
        p++;

        /* early exit for unreasonably long tags */
        if (len > 8) return 0;
    }

    /* length-based fast rejection */
    switch (len) {
    case 1: case 2: case 3: case 4:
    case 5: case 6: case 8:
        break;
    case 7:
        /* explicitly reject length 7 */
        return 0;
    default:
        /* length doesn't match any known inline tag */
        return 0;
    }

    /* extract first character once */
    const unsigned char first_char = (unsigned char)tag_name[0];

    /* optimized linear search with metadata filtering */
    for (int i = 0; inline_tags[i].tag; i++) {
        /* fast reject for first character mismatch */
        if (first_char != inline_tags[i].first_char) continue;

        /* reject by length mismatch */
        if (len != inline_tags[i].length) continue;

        /* final verification by exact string match */
        if (strcmp(tag_name, inline_tags[i].tag) == 0)
            return 1;
    }

    return 0;
}

int should_exclude_tag(const char* tag_name) {
    if (!tag_name || tag_name[0] == '\0') return 0;
    static const struct {
        const char* const tag;
        const size_t len;
        const unsigned char first;
    } excluded_tags[] = {
        {"script", 6, 's'}, {"style", 5, 's'}, {"link", 4, 'l'},
        {"meta", 4, 'm'}, {"head", 4, 'h'}, {"noscript", 8, 'n'},
        {"template", 8, 't'}, {"iframe", 6, 'i'}, {"form", 4, 'f'},
        {"input", 5, 'i'}, {"label", 5, 'l'}, {"canvas", 6, 'c'},
        {"svg", 3, 's'}, {"video", 5, 'v'}, {"source", 6, 's'},
        {"audio", 5, 'a'}, {"object", 6, 'o'}, {"button", 6, 'b'},
        {"map", 3, 'm'}, {"area", 4, 'a'}, {"frame", 5, 'f'},
        {"frameset", 8, 'f'}, {"noframes", 8, 'n'}, {"nav", 3, 'n'},
        {"picture", 7, 'p'}, {"progress", 8, 'p'}, {"select", 6, 's'},
        {"option", 6, 'o'}, {"param", 5, 'p'}, {"search", 6, 's'},
        {"samp", 4, 's'}, {"track", 5, 't'}, {"var", 3, 'v'},
        {"wbr", 3, 'w'}, {"mark", 4, 'm'}, {"meter", 5, 'm'},
        {"optgroup", 8, 'o'}, {"q", 1, 'q'}, {"blockquote", 10, 'b'},
        {"bdo", 3, 'b'}, {NULL, 0, 0}
    };

    /* compute the length with bounds checking */
    size_t len = 0;
    const char* p = tag_name;

    while (*p) {
        if (++len > 10) return 0;
        p++;
    }

    /* optimized length rejection */
    switch (len) {
    case 1: case 3: case 4: case 5:
    case 6: case 7: case 8: case 10:
        break;
    case 2: case 9:
        /* no excluded tags of these lengths */
        return 0;
    default:
        return 0;
    }

    /* extract first char once */
    const unsigned char first_char = (unsigned char)tag_name[0];

    /* perform an optimized search */
    for (int i = 0; excluded_tags[i].tag; i++) {
        /* first char mismatch */
        if (excluded_tags[i].first != first_char) continue;

        /* length mismatch */
        if (excluded_tags[i].len != len) continue;

        /* final verification */
        if (strcmp(tag_name, excluded_tags[i].tag) == 0)
            return 1;
    }

    return 0;
}

int is_whitespace_only(const char* text) {
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

    return 1;
}

int is_inside_table_cell(LaTeXConverter* converter, HTMLNode* node) {
    if (!node) return converter->state.in_table_cell;

    /* first check converter state for table cell */
    if (converter->state.in_table_cell) return 1;

    /* then check the node's parent hierarchy */
    HTMLNode* current = node->parent;

    while (current) {
        if (current->tag && (strcmp(current->tag, "td") == 0 || strcmp(current->tag, "th") == 0))
            return 1;

        current = current->parent;
    }

    return 0;
}

int is_inside_table(HTMLNode* node) {
    if (!node) return 0;
    HTMLNode* current = node->parent;

    while (current) {
        if (current->tag && strcmp(current->tag, "table") == 0)
            return 1;

        current = current->parent;
    }

    return 0;
}