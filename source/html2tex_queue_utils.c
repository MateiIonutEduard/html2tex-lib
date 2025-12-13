#include <stdlib.h>
#include "html2tex.h"

int queue_enqueue(NodeQueue** front, NodeQueue** rear, HTMLNode* data) {
    NodeQueue* node = (NodeQueue*)malloc(sizeof(NodeQueue));
    if (!node) return 0;

    node->data = data;
    node->next = NULL;

    if (*rear) (*rear)->next = node;
    else *front = node;

    *rear = node;
    return 1;
}

HTMLNode* queue_dequeue(NodeQueue** front, NodeQueue** rear) {
    NodeQueue* node = *front;
    if (!node) return NULL;

    HTMLNode* data = node->data;
    *front = node->next;

    if (!*front)
        *rear = NULL;

    free(node);
    return data;
}

void queue_cleanup(NodeQueue** front, NodeQueue** rear) {
    NodeQueue* current = *front;

    while (current) {
        NodeQueue* next = current->next;
        free(current);
        current = next;
    }

    *front = *rear = NULL;
}