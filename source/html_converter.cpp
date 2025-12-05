#include "html2tex.h"
#include "htmltex.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

HtmlTeXConverter::HtmlTeXConverter() : converter(nullptr, &html2tex_destroy), valid(false) {
    LaTeXConverter* raw_converter = html2tex_create();

    if (raw_converter) {
        converter.reset(raw_converter);
        valid = true;
    }
}

HtmlTeXConverter::HtmlTeXConverter(const HtmlTeXConverter& other) noexcept
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

bool HtmlTeXConverter::setDirectory(const std::string& fullPath) const noexcept {
    if (!converter || !valid) return false;
    html2tex_set_image_directory(converter.get(), fullPath.c_str());

    html2tex_set_download_images(converter.get(), 1);
    return true;
}

std::string HtmlTeXConverter::convert(const std::string& html) const {
    /* fast and optimized precondition checks */
    if (!converter || !valid)
        throw std::runtime_error("HtmlTeXConverter: Converter not initialized.");

    /* early return for empty input to avoid unnecessary allocations */
    if (html.empty())
        return "";

    /* convert HTML to LaTeX */
    char* const raw_result = html2tex_convert(converter.get(), html.c_str());

    /* handle nullptr result */
    if (!raw_result) {
        /* distinguish between error and empty conversion */
        if (hasError()) {
            const std::string error_msg = getErrorMessage();
            throw std::runtime_error("HTML to LaTeX conversion failed: " + error_msg);
        }

        /* empty result */
        return "";
    }

    /* ensures memory is freed even on exceptions */
    const auto deleter = [](char* p) noexcept { if (p) std::free(p); };
    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* check if result is actually empty */
    if (raw_result[0] == '\0')
        return "";

    /* measure length first to avoid double calculation */
    const std::size_t result_len = std::char_traits<char>::length(raw_result);

    /* return the result */
    return std::string(raw_result, result_len);
}

bool HtmlTeXConverter::convertToFile(const std::string& html, const std::string& filePath) const {
    /* validate converter and HTML code */
    if (!isValid()) 
        throw std::runtime_error("Converter not initialized.");

    if (html.empty()) 
        return false;

    /* convert the HTML code first */
    std::unique_ptr<char[], void(*)(char*)> result(
        html2tex_convert(converter.get(), html.c_str()),
        [](char* p) noexcept { 
            std::free(p); 
        });

    if (!result) {
        if (hasError()) throw std::runtime_error(getErrorMessage());
        
        /* empty but valid conversion */
        return false;
    }

    /* write the file */
    std::ofstream fout(filePath);

    if (!fout)
        throw std::runtime_error("Cannot open output file.");

    fout << result.get();
    fout.flush();

    if (!fout)
        throw std::runtime_error("Failed to write LaTeX output.");

    return true;
}

std::string HtmlTeXConverter::convert(const HtmlParser& parser) const {
    /* precondition validation */
    if (!isValid())
        throw std::runtime_error("HtmlTeXConverter in invalid state.");

    /* get HTML content - one serialization only */
    std::string html = parser.toString();

    /* return empty string for empty input */
    if (html.empty()) return "";

    /* perform conversion */
    char* raw_result = html2tex_convert(converter.get(), html.c_str());

    /* RAII management with custom deleter */
    const auto deleter = [](char* p) noexcept { std::free(p); };
    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* error analysis */
    if (!raw_result) {
        if (hasError()) {
            throw std::runtime_error(
                "HTML to LaTeX conversion failed: " + getErrorMessage());
        }

        /* valid empty result */
        return "";
    }

    /* return result (compiler will optimize it) */
    return std::string(raw_result);
}

bool HtmlTeXConverter::convertToFile(const HtmlParser& parser, const std::string& filePath) const {
    /* fast precondition validation */
    if (!converter || !valid)
        throw std::runtime_error("HtmlTeXConverter in invalid state.");

    /* early exit for empty parser */
    if (!parser.hasContent())
        return false;

    /* get serialized HTML - only serialize once */
    const std::string html = parser.toString();

    /* validate serialized content */
    if (html.empty())
        return false;

    /* convert HTML to LaTeX */
    char* raw_result = html2tex_convert(converter.get(), html.c_str());

    /* RAII with custom deleter */
    const auto deleter = [](char* p) noexcept {
        if (p) std::free(p);
    };

    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* check for conversion errors */
    if (!raw_result) {
        if (hasError()) {
            throw std::runtime_error(
                "HTML to LaTeX conversion failed: " + getErrorMessage());
        }

        /* valid empty conversion */
        return false;
    }

    /* check if result is empty */
    if (raw_result[0] == '\0')
        return false;

    /* open file with optimal flags */
    std::ofstream fout;

    /* config stream for maximum performance */
    fout.rdbuf()->pubsetbuf(nullptr, 0);
    fout.open(filePath, std::ios::binary | std::ios::trunc);

    if (!fout)
        throw std::runtime_error("Cannot open output file: " + filePath);

    /* get result length once */
    const size_t result_len = std::strlen(raw_result);

    /* write entire result in one operation */
    if (!fout.write(raw_result, static_cast<std::streamsize>(result_len)))
        throw std::runtime_error("Failed to write LaTeX output to: " + filePath);

    /* explicit flush to ensure data is written */
    fout.flush();

    /* final check */
    if (!fout)
        throw std::runtime_error("Failed to flush LaTeX output to: " + filePath);

    return true;
}

bool HtmlTeXConverter::convertToFile(const HtmlParser& parser, std::ofstream& output) const {
    /* fast precondition validation */
    if (!converter || !valid)
        throw std::runtime_error("HtmlTeXConverter: Converter not initialized.");

    /* early exit for empty parser */
    if (!parser.hasContent())
        return false;

    /* get serialized HTML - only serialize once */
    const std::string html = parser.toString();

    /* validate serialized content */
    if (html.empty())
        return false;

    /* convert HTML to LaTeX */
    char* raw_result = html2tex_convert(converter.get(), html.c_str());

    /* RAII with custom deleter */
    const auto deleter = [](char* p) noexcept {
        /* free handles nullptr */
        std::free(p);
    };

    std::unique_ptr<char[], decltype(deleter)> result_guard(raw_result, deleter);

    /* check for conversion errors */
    if (!raw_result) {
        if (hasError()) {
            throw std::runtime_error(
                std::string("HTML to LaTeX conversion failed: ") + getErrorMessage());
        }

        /* valid empty conversion */
        return false;
    }

    /* check if result is empty */
    if (raw_result[0] == '\0')
        return false;

    /* get result length once */
    const std::size_t result_len = std::strlen(raw_result);

    /* write entire result in one operation */
    if (!output.write(raw_result, static_cast<std::streamsize>(result_len)))
        throw std::runtime_error("Failed to write LaTeX output to stream.");

    /* ensure data is written */
    output.flush();

    if (!output)
        throw std::runtime_error("Failed to flush LaTeX output to stream.");

    return true;
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

HtmlTeXConverter& HtmlTeXConverter::operator =(const HtmlTeXConverter& other) noexcept {
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