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
    void setParent(std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node) noexcept;

public:
    /* Create an empty, valid parser instance. */
    HtmlParser();

    /* Creates a parser from the input HTML. */
    explicit HtmlParser(const std::string&);

    /* Initializes parser from HTML with optimization flag. */
    HtmlParser(const std::string&, int) noexcept;

    /* Initializes the parser with the input DOM tree. */
    explicit HtmlParser(HTMLNode*);

    /* Instantiates the parser with the DOM tree and the minify option. */
    HtmlParser(HTMLNode*, int) noexcept;

    /* Clones an existing HtmlParser to create a new parser. */
    HtmlParser(const HtmlParser&) noexcept;

    /* Efficiently moves an existing HtmlParser instance. */
    HtmlParser(HtmlParser&&) noexcept;

    /* Return a pointer to the DOM tree’s root node. */
    HTMLNode* getHtmlNode() const noexcept;

    /* Check whether the parser contains content. */
    bool hasContent() const noexcept;

    HtmlParser& operator =(const HtmlParser&);
    HtmlParser& operator =(HtmlParser&&) noexcept;

    friend std::ostream& operator <<(std::ostream&, const HtmlParser&);
    friend std::istream& operator >>(std::istream&, HtmlParser&);

    /* Initializes the parser from the given file stream. */
    static HtmlParser fromStream(std::ifstream&) noexcept;

    /* Creates a parser from the given HTML file path. */
    static HtmlParser fromHtml(const std::string&) noexcept;

    /* Write the DOM tree in HTML format to the file at the specified path. */
    void writeTo(const std::string&) const;

    /* Returns prettified HTML from this instance. */
    std::string toString() const noexcept;
    ~HtmlParser() = default;
};

class HtmlTeXConverter {
private:
    std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> converter;
    bool valid;

public:
    /* Create a new valid HtmlTeXConverter instance. */
    HtmlTeXConverter();
    ~HtmlTeXConverter() = default;

    /* Convert the input HTML code to the corresponding LaTeX output. */
    std::string convert(const std::string&) const;

    /* Convert the HtmlParser instance to its corresponding LaTeX output. */
    std::string convert(const HtmlParser&) const;

    /* Convert the input HTML code to LaTeX and write the output to the file at the specified path. */
    bool convertToFile(const std::string&, const std::string&) const;

    /* Convert the HtmlParser instance to LaTeX and write the result to the specified file path. */
    bool convertToFile(const HtmlParser&, const std::string&) const;

    /* Convert the HtmlParser instance to LaTeX and write the result to a file. */
    bool convertToFile(const HtmlParser&, std::ofstream&) const;

    /*
       Set the directory where images extracted from the DOM tree are saved.
       @return true on success, false otherwise.
    */
    bool setDirectory(const std::string&) const noexcept;

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
    HtmlTeXConverter(const HtmlTeXConverter&) noexcept;

    HtmlTeXConverter& operator =(const HtmlTeXConverter&) noexcept;
    HtmlTeXConverter& operator =(HtmlTeXConverter&&) noexcept;
};

#endif