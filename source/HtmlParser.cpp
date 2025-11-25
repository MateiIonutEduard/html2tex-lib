#include "html_parser.h"
#include "html2tex.h"
#include <stdlib.h>
#include <string>
#include <iostream>
using namespace std;

HtmlParser::HtmlParser(const string& html) {
	node = html2tex_parse(html.c_str());
}

HtmlParser::HtmlParser(const HtmlParser& parser) {
    node = dom_tree_copy(parser.node);
}

HtmlParser& HtmlParser::operator=(const HtmlParser& parser)
{
    HtmlParser newParser = HtmlParser(parser);
    return newParser;
}

HTMLNode* HtmlParser::dom_tree_copy(const HTMLNode* node) {
    return dom_tree_copy(node, NULL);
}

HTMLNode* HtmlParser::dom_tree_copy(const HTMLNode* node, HTMLNode* parent) {
    if (!node) return NULL;

    HTMLNode* new_node = (HTMLNode*)malloc(sizeof(HTMLNode));
    if (!new_node) return NULL;

    /* copy basic fields */
    new_node->tag = node->tag ? strdup(node->tag) : NULL;
    new_node->content = node->content ? strdup(node->content) : NULL;

    new_node->parent = parent;
    new_node->next = NULL;
    new_node->children = NULL;

    /* deep copy attributes */
    new_node->attributes = NULL;

    HTMLAttribute** current_attr = &new_node->attributes;
    HTMLAttribute* orig_attr = node->attributes;

    while (orig_attr) {
        HTMLAttribute* new_attr = (HTMLAttribute*)malloc(sizeof(HTMLAttribute));

        if (!new_attr) {
            html2tex_free_node(new_node);
            return NULL;
        }

        new_attr->key = strdup(orig_attr->key);
        new_attr->value = orig_attr->value ? strdup(orig_attr->value) : NULL;

        new_attr->next = NULL;
        *current_attr = new_attr;

        current_attr = &new_attr->next;
        orig_attr = orig_attr->next;
    }

    /* deep copy children recursively */
    HTMLNode** current_child = &new_node->children;
    HTMLNode* orig_child = node->children;

    while (orig_child) {
        HTMLNode* new_child = dom_tree_copy(orig_child, new_node);

        if (!new_child) {
            html2tex_free_node(new_node);
            return NULL;
        }

        *current_child = new_child;
        current_child = &new_child->next;
        orig_child = orig_child->next;
    }

    return new_node;
}

HtmlParser::~HtmlParser() {
	if(node)
		html2tex_free_node(node);
}