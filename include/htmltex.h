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
    int minify;

    HTMLNode* dom_tree_copy(HTMLNode*);
    void setParent(HTMLNode*);

public:
    HtmlParser();

    /* Create a parser from the input HTML. */
    HtmlParser(const string&);

    /* Initialize parser from HTML; use second parameter to parse DOM efficiently. */
    HtmlParser(const string&, int);

    /* Instantiate parser with the input DOM tree. */
    HtmlParser(HTMLNode*);

    /* Instantiate parser by cloning another HtmlParser instance. */
    HtmlParser(const HtmlParser&);

    HtmlParser& operator =(const HtmlParser&);
    friend ostream& operator <<(ostream&, HtmlParser&);
    friend istream& operator >>(istream&, HtmlParser&);

    /* Get pretty HTML from the object. */
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

    /* Check for errors during conversion. */
    bool hasError() const;

    /* Return the conversion error code. */
    int getErrorCode() const;

    /* Return the conversion error message. */
    string getErrorMessage() const;

    /* delete copy constructor and assignment operator to prevent copying */
    HtmlTeXConverter(const HtmlTeXConverter&) = delete;
    HtmlTeXConverter& operator=(const HtmlTeXConverter&) = delete;
};

#endif