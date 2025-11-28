#include "html2tex.h"
#include "htmltex.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

HtmlParser::HtmlParser() : node(nullptr, &html2tex_free_node), minify(0) 
{ }

HtmlParser::HtmlParser(const string& html) : HtmlParser(html, 0) { }

HtmlParser::HtmlParser(const string& html, int minify_flag)
    : node(nullptr, &html2tex_free_node), minify(minify_flag) {
    HTMLNode* raw_node = minify_flag ? html2tex_parse_minified(html.c_str())
        : html2tex_parse(html.c_str());

    if (raw_node) node.reset(raw_node);
}

HtmlParser::HtmlParser(HTMLNode* raw_node) : HtmlParser(raw_node, 0)
{ }

HtmlParser::HtmlParser(HTMLNode* raw_node, int minify_flag)
    : node(nullptr, &html2tex_free_node), minify(minify_flag) {
    if (raw_node) {
        HTMLNode* copied_node = dom_tree_copy(raw_node);
        if (copied_node) node.reset(copied_node);
    }
}

HtmlParser::HtmlParser(const HtmlParser& parser)
    : node(nullptr, &html2tex_free_node), minify(parser.minify) {
    if (parser.node) {
        HTMLNode* copied_node = dom_tree_copy(parser.node.get());
        if (copied_node) node.reset(copied_node);
    }
}

HtmlParser& HtmlParser::operator=(const HtmlParser& parser) {
    if (this != &parser) {
        if (parser.node) {
            HTMLNode* copied_node = dom_tree_copy(parser.node.get());
            if (copied_node) node.reset(copied_node);
            else node.reset();
        }
        else node.reset();
        minify = parser.minify;
    }

    return *this;
}

ostream& operator<<(ostream& out, HtmlParser& parser) {
    string output = parser.toString();
    out << output << endl;
    return out;
}

void HtmlParser::setParent(unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node) {
    node = move(new_node);
}

istream& operator>>(istream& in, HtmlParser& parser) {
    ostringstream stream;
    stream << in.rdbuf();

    string html_content = stream.str();
    HTMLNode* raw_node = parser.minify ? html2tex_parse_minified(html_content.c_str())
        : html2tex_parse(html_content.c_str());

    if (raw_node)
        parser.setParent(unique_ptr<HTMLNode, 
            decltype(&html2tex_free_node)>(raw_node, &html2tex_free_node));

    return in;
}

string HtmlParser::toString() {
    if (!node) return "";
    char* output = get_pretty_html(node.get());

    if (output) {
        string result(output);
        free(output);
        return result;
    }

    return "";
}