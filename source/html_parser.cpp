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
    /* empty parser, but valid state */
    if (html.empty()) return;

    HTMLNode* raw_node = minify_flag ? html2tex_parse_minified(html.c_str())
        : html2tex_parse(html.c_str());

    /* if parsing fails, node remains nullptr */
    if (raw_node) node.reset(raw_node);
}

HtmlParser::HtmlParser(HTMLNode* raw_node) : HtmlParser(raw_node, 0)
{ }

HtmlParser::HtmlParser(HTMLNode* raw_node, int minify_flag)
    : node(nullptr, &html2tex_free_node), minify(minify_flag) {
    /* object is in empty but valid state */
    if (!raw_node) return;

    HTMLNode* copied_node = dom_tree_copy(raw_node);
    if (copied_node) node.reset(copied_node);
}

HtmlParser::HtmlParser(const HtmlParser& other)
    : node(nullptr, &html2tex_free_node), minify(other.minify) {
    if (other.node) {
        HTMLNode* copied_node = dom_tree_copy(other.node.get());
        if (copied_node) node.reset(copied_node);
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
    /* check if stream is in good state first */
    if (!in.good()) {
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(nullptr, &html2tex_free_node));
        return in;
    }

    std::ostringstream stream;
    char buffer[4096];

    /* read stream in chunks until EOF or failure */
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
        stream.write(buffer, in.gcount());
        if (in.eof()) break;
    }

    /* check if we actually read anything */
    std::string html_content = stream.str();

    if (html_content.empty()) {
        /* clear failure state for future reads */
        if (in.fail() && !in.eof()) in.clear();

        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(nullptr, &html2tex_free_node));
        return in;
    }

    /* only check EOF state after ensuring that content exists */
    if (!in.eof() && in.fail()) in.clear();

    HTMLNode* raw_node = parser.minify ?
        html2tex_parse_minified(html_content.c_str()) :
        html2tex_parse(html_content.c_str());

    if (raw_node) {
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(raw_node, &html2tex_free_node));
    }
    else {
        /* reset to empty state if parsing fails */
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(nullptr, &html2tex_free_node));
    }

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