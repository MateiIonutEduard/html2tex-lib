#ifndef HTMLTEX_H
#define HTMLTEX_H

#include <memory>
#include <string>
#include "html2tex.h"
#include <iostream>
using namespace std;

class HtmlParser {
private:
    unique_ptr<HTMLNode, decltype(&html2tex_free_node)> node;
    int minify;
    void setParent(unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node);

public:
    HtmlParser();

    /* Creates a parser from the input HTML. */
    HtmlParser(const string&);

    /* Initializes parser from HTML with optimization flag. */
    HtmlParser(const string&, int);

    /* Initializes the parser with the input DOM tree. */
    HtmlParser(HTMLNode*);

    /* Instantiates the parser with the DOM tree and the minify option. */
    HtmlParser(HTMLNode* node, int);

    /* Clones an existing HtmlParser to create a new parser. */
    HtmlParser(const HtmlParser&);

    HtmlParser& operator=(const HtmlParser&);
    friend ostream& operator<<(ostream&, HtmlParser&);
    friend istream& operator>>(istream&, HtmlParser&);

    /* Returns prettified HTML from this instance. */
    string toString();
    ~HtmlParser() = default;
};

class HtmlTeXConverter {
private:
    unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> converter;

public:
    HtmlTeXConverter();
    ~HtmlTeXConverter() = default;

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