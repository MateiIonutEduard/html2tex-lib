#include <stdio.h>
#include "html2tex.h"

inline int queue_enqueue(NodeQueue** front, NodeQueue** rear, HTMLNode* data) {
    NodeQueue* node = malloc(sizeof(NodeQueue));
    if (!node) return 0;

    node->data = data;
    node->next = NULL;

    if (*rear) (*rear)->next = node;
    else *front = node;

    *rear = node;
    return 1;
}

inline HTMLNode* queue_dequeue(NodeQueue** front, NodeQueue** rear) {
    NodeQueue* node = *front;
    if (!node) return NULL;

    HTMLNode* data = node->data;
    *front = node->next;

    if (!*front)
        *rear = NULL;

    free(node);
    return data;
}

inline void queue_cleanup(NodeQueue** front, NodeQueue** rear) {
    NodeQueue* current = *front;

    while (current) {
        NodeQueue* next = current->next;
        free(current);
        current = next;
    }

    *front = *rear = NULL;
}