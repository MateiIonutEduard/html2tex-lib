#include "html2tex.h"
#include "HtmlToLatexConverter.h"
#include <iostream>
using namespace std;

class HtmlToLatexConverter::Impl {
public:
    Impl() : converter(html2tex_create()) {}
    ~Impl() {
        if (converter) {
            html2tex_destroy(converter);
        }
    }

    LaTeXConverter* converter;
};

HtmlToLatexConverter::HtmlToLatexConverter() : pImpl(make_unique<Impl>()) { }

HtmlToLatexConverter::~HtmlToLatexConverter() = default;

string HtmlToLatexConverter::convert(string& html) {
    if (!pImpl->converter) 
        return "";

    char* result = html2tex_convert(pImpl->converter, 
        html.c_str());

    if (result) {
        string latex(result);
        free(result);
        return latex;
    }

    return "";
}

bool HtmlToLatexConverter::hasError() const {
    return pImpl->converter && 
        html2tex_get_error(pImpl->converter) != 0;
}

int HtmlToLatexConverter::getErrorCode() const {
    return pImpl->converter ? 
        html2tex_get_error(pImpl->converter) 
        : -1;
}

string HtmlToLatexConverter::getErrorMessage() const {
    if (!pImpl->converter) return "Converter not initialized";
    return html2tex_get_error_message(pImpl->converter);
}