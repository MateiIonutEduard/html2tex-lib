#include "html2tex.h"
#include <string>
#include <iostream>
using namespace std;

class HtmlParser {
	HTMLNode* node;
	HTMLNode* dom_tree_copy(const HTMLNode*);
	HTMLNode* dom_tree_copy(const HTMLNode*, HTMLNode*);

public:
	HtmlParser(const string&);
	HtmlParser(const HtmlParser&);
	HtmlParser& operator =(const HtmlParser&);
	~HtmlParser();
};