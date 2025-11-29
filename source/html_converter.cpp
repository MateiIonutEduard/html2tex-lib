#include "html2tex.h"
#include "htmltex.h"
#include <iostream>
#include <stdexcept>

HtmlTeXConverter::HtmlTeXConverter() : converter(nullptr, &html2tex_destroy), valid(false) {
    LaTeXConverter* raw_converter = html2tex_create();

    if (raw_converter) {
        converter.reset(raw_converter);
        valid = true;
    }
}

HtmlTeXConverter::HtmlTeXConverter(const HtmlTeXConverter& other) 
: converter(nullptr, &html2tex_destroy), valid(false) {
    if (other.converter && other.valid) {
        LaTeXConverter* clone = html2tex_copy(other.converter.get());

        if (clone) {
            converter.reset(clone);
            valid = true;
        }
    }
}

HtmlTeXConverter::HtmlTeXConverter(HtmlTeXConverter&& other) noexcept
    : converter(std::move(other.converter)), valid(other.valid) { 
    other.converter.reset(nullptr);
    other.valid = false;
}

bool HtmlTeXConverter::setDirectory(const std::string& fullPath) {
    if (!converter || !valid) return false;
    html2tex_set_image_directory(converter.get(), fullPath.c_str());

    html2tex_set_download_images(converter.get(), 1);
    return true;
}

std::string HtmlTeXConverter::convert(const std::string& html) {
    if (!converter || !valid)
        throw std::runtime_error("Converter not initialized.");

    if (html.empty()) return "";
    char* result = html2tex_convert(converter.get(), html.c_str());

    if (!result) {
        /* check if this is an actual error or just empty conversion */
        if (hasError()) throw std::runtime_error(getErrorMessage());

        /* legitimate empty result */
        return "";
    }

    std::string latex(result);
    free(result);
    return latex;
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

bool HtmlTeXConverter::isValid() const { return valid; }

HtmlTeXConverter& HtmlTeXConverter::operator =(const HtmlTeXConverter& other) {
    if (this != &other) {
        std::unique_ptr<LaTeXConverter, decltype(&html2tex_destroy)> temp(nullptr, &html2tex_destroy);
        bool new_valid = false;

        if (other.converter && other.valid) {
            LaTeXConverter* clone = html2tex_copy(other.converter.get());

            if (clone) {
                temp.reset(clone);
                new_valid = true;
            }
        }

        converter = std::move(temp);
        valid = new_valid;
    }

    return *this;
}

HtmlTeXConverter& HtmlTeXConverter::operator =(HtmlTeXConverter&& other) noexcept {
    if (this != &other) {
        converter = std::move(other.converter);
        valid = other.valid;
        other.valid = false;
    }

    return *this;
}