#ifndef HTML_TO_LATEX_CONVERTER_H
#define HTML_TO_LATEX_CONVERTER_H

#include <memory>
#include <string>
#include "html2tex.h"
#include <iostream>
using namespace std;

class HtmlToLatexConverter {
public:
    HtmlToLatexConverter();
    ~HtmlToLatexConverter();

    /* convert HTML string to LaTeX */
    string convert(const string& html);

    /* check for errors */
    bool hasError() const;
    int getErrorCode() const;
    string getErrorMessage() const;

    /* delete copy constructor and assignment operator to prevent copying */
    HtmlToLatexConverter(const HtmlToLatexConverter&) = delete;
    HtmlToLatexConverter& operator=(const HtmlToLatexConverter&) = delete;

private:
    class Impl;
    unique_ptr<Impl> pImpl;
};

#endif