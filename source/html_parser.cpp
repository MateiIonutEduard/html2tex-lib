#include "html2tex.h"
#include "htmltex.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

HtmlParser::HtmlParser() {
    node = NULL;
    minify = 0;
}

HtmlParser::HtmlParser(const string& html) : HtmlParser(html, 0)
{ }

HtmlParser::HtmlParser(const string& html, int minify) {
	node = minify ? html2tex_parse_minified(html.c_str()) 
        : html2tex_parse(html.c_str());
    this->minify = minify;
}

HtmlParser::HtmlParser(HTMLNode* node) {
    node = dom_tree_copy(node);
}

HtmlParser::HtmlParser(const HtmlParser& parser) {
    node = dom_tree_copy(parser.node);
}

HtmlParser& HtmlParser::operator=(const HtmlParser& parser)
{
    HtmlParser newParser = HtmlParser(parser);
    return newParser;
}

ostream& operator <<(ostream& out, HtmlParser& parser) {
    string output = parser.toString();
    out << output << endl;
    return out;
}

void HtmlParser::setParent(HTMLNode* node) {
    if (this->node) html2tex_free_node(this->node);
    this->node = dom_tree_copy(node);
}

istream& operator >>(istream& in, HtmlParser& parser) {
    
    ostringstream stream;
    stream << in.rdbuf();

    string ptr = stream.str();
    HTMLNode* node = parser.minify ? html2tex_parse_minified(ptr.c_str())
        : html2tex_parse(ptr.c_str());

    if (node) {
        parser.setParent(node);
        html2tex_free_node(node);
    }

    return in;
}

string HtmlParser::toString() {
    char* output = get_pretty_html(node);

    if (output) {
        string result(output);
        free(output);
        return result;
    }

    return "";
}

HTMLNode* HtmlParser::dom_tree_copy(HTMLNode* node) {
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

HtmlParser::~HtmlParser() {
	if(node)
		html2tex_free_node(node);
}