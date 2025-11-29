#include "html2tex.h"
#include "htmltex.h"
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>

HtmlParser::HtmlParser() : node(nullptr, &html2tex_free_node), minify(0) 
{ }

HtmlParser::HtmlParser(const std::string& html) : HtmlParser(html, 0) { }

HtmlParser::HtmlParser(const std::string& html, int minify_flag)
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
        else node.reset(nullptr);
    }
}

HtmlParser::HtmlParser(const HtmlParser& other)
    : node(nullptr, &html2tex_free_node), minify(other.minify) {
    if (other.node) {
        HTMLNode* copied_node = dom_tree_copy(other.node.get());
        if (copied_node) node.reset(copied_node);
        else node.reset(nullptr);
    }
}

HtmlParser::HtmlParser(HtmlParser&& other) noexcept
    : node(std::move(other.node)), minify(other.minify) {
    other.node.reset(nullptr);
    other.minify = 0;
}

HtmlParser& HtmlParser::operator =(const HtmlParser& other) {
    if (this != &other) {
        /* temp smart pointer for exception safety */
        std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> temp(nullptr, &html2tex_free_node);

        if (other.node) {
            HTMLNode* copied_node = dom_tree_copy(other.node.get());
            if (copied_node) temp.reset(copied_node);
        }

        /* commit changes */
        node = std::move(temp);
        minify = other.minify;
    }

    return *this;
}

HtmlParser& HtmlParser::operator =(HtmlParser&& other) noexcept {
    if (this != &other) {
        node = std::move(other.node);
        minify = other.minify;
        other.minify = 0;
    }

    return *this;
}

std::ostream& operator <<(std::ostream& out, const HtmlParser& parser) {
    std::string output = parser.toString();
    out << output;
    return out;
}

void HtmlParser::setParent(std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node) {
    node = std::move(new_node);
}

std::istream& operator >>(std::istream& in, HtmlParser& parser) {
    std::ostringstream stream;
    stream << in.rdbuf();

    std::string html_content = stream.str();
    HTMLNode* raw_node = parser.minify ? html2tex_parse_minified(html_content.c_str())
        : html2tex_parse(html_content.c_str());

    if (raw_node)
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(raw_node, &html2tex_free_node));

    return in;
}

std::string HtmlParser::toString() const {
    if (!node) return "";
    char* output = get_pretty_html(node.get());

    /* additional safety check */
    if (!output) return "";
    std::string result(output);

    free(output);
    return result;
}