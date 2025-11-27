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

HtmlParser::HtmlParser(HTMLNode* node) : HtmlParser(node, 0)
{  }

HtmlParser::HtmlParser(HTMLNode* node, int minify) {
    node = dom_tree_copy(node);
    this->minify = minify;
}

HtmlParser::HtmlParser(const HtmlParser& parser) {
    node = dom_tree_copy(parser.node);
    minify = parser.minify;
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

HtmlParser::~HtmlParser() {
	if(node)
		html2tex_free_node(node);
}