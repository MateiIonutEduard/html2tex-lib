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
    explicit HtmlParser(const std::string&);

    /* Initializes parser from HTML with optimization flag. */
    HtmlParser(const std::string&, int);

    /* Initializes the parser with the input DOM tree. */
    explicit HtmlParser(HTMLNode*);

    /* Instantiates the parser with the DOM tree and the minify option. */
    HtmlParser(HTMLNode*, int);

    /* Clones an existing HtmlParser to create a new parser. */
    HtmlParser(const HtmlParser&);

    /* Efficiently moves an existing HtmlParser instance. */
    HtmlParser(HtmlParser&&) noexcept;

    HtmlParser& operator =(const HtmlParser&);
    HtmlParser& operator =(HtmlParser&&) noexcept;

    friend std::ostream& operator <<(std::ostream&, const HtmlParser&);
    friend std::istream& operator >>(std::istream&, HtmlParser&);

    /* Initializes the parser from the given file stream. */
    static HtmlParser FromStream(std::ifstream&);

    /* Returns prettified HTML from this instance. */
    std::string toString() const;
    ~HtmlParser() = default;
};

class HtmlTeXConverter {
private:
    std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> converter;
    bool valid;

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

    /* Check whether the converter is initialized and valid. */
    bool isValid() const;

    /* Efficiently moves an existing HtmlTeXConverter instance. */
    HtmlTeXConverter(HtmlTeXConverter&&) noexcept;

    /* Creates a new HtmlTeXConverter by copying an existing instance. */
    HtmlTeXConverter(const HtmlTeXConverter&);

    HtmlTeXConverter& operator =(const HtmlTeXConverter&);
    HtmlTeXConverter& operator =(HtmlTeXConverter&&) noexcept;
};

#endif