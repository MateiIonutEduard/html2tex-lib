#ifndef HTMLTEX_H
#define HTMLTEX_H

#include <memory>
#include <string>
#include "html2tex.h"
#include <iostream>
using namespace std;

class HtmlParser {
private:
    HTMLNode* node;

    HTMLNode* dom_tree_copy(const HTMLNode*);
    HTMLNode* dom_tree_copy(const HTMLNode*, HTMLNode*);

public:
    HtmlParser(const string&);
    HtmlParser(const string&, bool);
    HtmlParser(HTMLNode*);
    HtmlParser(const HtmlParser&);

    HtmlParser& operator =(const HtmlParser&);
    friend ostream& operator<<(ostream&, HtmlParser&);

    string toString();
    ~HtmlParser();
};

class HtmlTeXConverter {
private:
    LaTeXConverter* converter;

public:
    HtmlTeXConverter();
    ~HtmlTeXConverter();

    /* Convert the input HTML code to the corresponding LaTeX output. */
    string convert(const string&);

    /*
       Set the directory where images extracted from the DOM tree are saved.
       @return true on success, false otherwise.
    */
    bool setDirectory(const string&);

    /* check for errors */
    bool hasError() const;
    int getErrorCode() const;
    string getErrorMessage() const;

    /* delete copy constructor and assignment operator to prevent copying */
    HtmlTeXConverter(const HtmlTeXConverter&) = delete;
    HtmlTeXConverter& operator=(const HtmlTeXConverter&) = delete;
};

#endif