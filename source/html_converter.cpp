#include "html2tex.h"
#include "htmltex.h"
#include <iostream>

HtmlTeXConverter::HtmlTeXConverter() : converter(nullptr, &html2tex_destroy) {
    LaTeXConverter* raw_converter = html2tex_create();
    if (raw_converter) converter.reset(raw_converter);
}

HtmlTeXConverter::HtmlTeXConverter(HtmlTeXConverter&& other) noexcept
    : converter(std::move(other.converter)) { }

bool HtmlTeXConverter::setDirectory(const std::string& fullPath) {
    if (converter) {
        html2tex_set_image_directory(converter.get(), fullPath.c_str());
        html2tex_set_download_images(converter.get(), 1);
        return true;
    }

    return false;
}

std::string HtmlTeXConverter::convert(const std::string& html) {
    if (!converter) return "";
    char* result = html2tex_convert(converter.get(), html.c_str());

    if (result) {
        std::string latex(result);
        free(result);

        html2tex_reset(converter.get());
        return latex;
    }

    return "";
}

bool HtmlTeXConverter::hasError() const {
    return converter && html2tex_get_error(converter.get()) != 0;
}

int HtmlTeXConverter::getErrorCode() const {
    return converter ? html2tex_get_error(converter.get()) : -1;
}

std::string HtmlTeXConverter::getErrorMessage() const {
    if (!converter) return "Converter not initialized.";
    return html2tex_get_error_message(converter.get());
}

HtmlTeXConverter& HtmlTeXConverter::operator=(HtmlTeXConverter&& other) noexcept {
    if (this != &other) {
        /* free current resources */
        converter.reset();

        /* transfer the ownership */
        converter = std::move(other.converter);
    }

    return *this;
}