#ifndef HTMLTEX_H
#define HTMLTEX_H

#include <memory>
#include <string>
#include "html2tex.h"
#include <iostream>

class HtmlParser {
private:
    std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> node;
    int minify;
    void setParent(std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node);

public:
    HtmlParser();

    /* Creates a parser from the input HTML. */
    HtmlParser(const std::string&);

    /* Initializes parser from HTML with optimization flag. */
    HtmlParser(const std::string&, int);

    /* Initializes the parser with the input DOM tree. */
    HtmlParser(HTMLNode*);

    /* Instantiates the parser with the DOM tree and the minify option. */
    HtmlParser(HTMLNode* node, int);

    /* Clones an existing HtmlParser to create a new parser. */
    HtmlParser(const HtmlParser&);

    /* Efficiently moves an existing HtmlParser instance. */
    HtmlParser(HtmlParser&&) noexcept;

    HtmlParser& operator=(const HtmlParser&);
    HtmlParser& operator=(HtmlParser&&) noexcept;

    friend std::ostream& operator<<(std::ostream&, HtmlParser&);
    friend std::istream& operator>>(std::istream&, HtmlParser&);

    /* Returns prettified HTML from this instance. */
    std::string toString();
    ~HtmlParser() = default;
};

class HtmlTeXConverter {
private:
    std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> converter;

public:
    HtmlTeXConverter();
    ~HtmlTeXConverter() = default;

    /* Convert the input HTML code to the corresponding LaTeX output. */
    std::string convert(const std::string&);

    /*
       Set the directory where images extracted from the DOM tree are saved.
       @return true on success, false otherwise.
    */
    bool setDirectory(const std::string&);

    /* Check for errors during conversion. */
    bool hasError() const;

    /* Return the conversion error code. */
    int getErrorCode() const;

    /* Return the conversion error message. */
    std::string getErrorMessage() const;

    /* Efficiently moves an existing HtmlTeXConverter instance. */
    HtmlTeXConverter(HtmlTeXConverter&&) noexcept;

    /* delete copy constructor and assignment operator to prevent copying */
    HtmlTeXConverter(const HtmlTeXConverter&) = delete;

    HtmlTeXConverter& operator=(const HtmlTeXConverter&) = delete;
    HtmlTeXConverter& operator=(HtmlTeXConverter&& other) noexcept;
};

#endif