#include "html2tex.h"
#include "htmltex.h"
#include <iostream>
using namespace std;

HtmlTeXConverter::HtmlTeXConverter() {
    converter = html2tex_create();
}

bool HtmlTeXConverter::setDirectory(const string& fullPath) {
    if (converter) {
        html2tex_set_image_directory(converter, fullPath.c_str());
        html2tex_set_download_images(converter, 1);
        return true;
    }

    return false;
}

string HtmlTeXConverter::convert(const string& html) {
    if (!converter) return "";

    char* result = html2tex_convert(converter, html.c_str());

    if (result) {
        string latex(result);
        free(result);
        return latex;
    }

    return "";
}

bool HtmlTeXConverter::hasError() const {
    return converter && html2tex_get_error(converter) != 0;
}

int HtmlTeXConverter::getErrorCode() const {
    return converter ? html2tex_get_error(converter) : -1;
}

string HtmlTeXConverter::getErrorMessage() const {
    if (!converter) return "Converter not initialized";
    return html2tex_get_error_message(converter);
}

HtmlTeXConverter::~HtmlTeXConverter() {
    if (converter)
        html2tex_destroy(converter);
}