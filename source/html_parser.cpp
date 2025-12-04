#include "html2tex.h"
#include "htmltex.h"
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

HtmlParser::HtmlParser() : node(nullptr, &html2tex_free_node), minify(0) 
{ }

HtmlParser::HtmlParser(const std::string& html) : HtmlParser(html, 0) { }

HtmlParser::HtmlParser(const std::string& html, int minify_flag)
    : node(nullptr, &html2tex_free_node), minify(minify_flag) {
    /* empty parser, but valid state */
    if (html.empty()) return;

    HTMLNode* raw_node = minify_flag ? html2tex_parse_minified(html.c_str())
        : html2tex_parse(html.c_str());

    /* if parsing fails, node remains nullptr */
    if (raw_node) node.reset(raw_node);
}

HtmlParser::HtmlParser(HTMLNode* raw_node) : HtmlParser(raw_node, 0)
{ }

HtmlParser::HtmlParser(HTMLNode* raw_node, int minify_flag)
    : node(nullptr, &html2tex_free_node), minify(minify_flag) {
    /* object is in empty but valid state */
    if (!raw_node) return;

    HTMLNode* copied_node = dom_tree_copy(raw_node);
    if (copied_node) node.reset(copied_node);
}

HtmlParser::HtmlParser(const HtmlParser& other)
    : node(nullptr, &html2tex_free_node), minify(other.minify) {
    if (other.node) {
        HTMLNode* copied_node = dom_tree_copy(other.node.get());
        if (copied_node) node.reset(copied_node);
    }
}

HtmlParser::HtmlParser(HtmlParser&& other) noexcept
    : node(std::move(other.node)), minify(other.minify) {
    other.node.reset(nullptr);
    other.minify = 0;
}

HtmlParser& HtmlParser::operator =(const HtmlParser& other) {
    if (this != &other) {
        /* temp smart pointer for exception safety */
        std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> temp(nullptr, &html2tex_free_node);

        if (other.node) {
            HTMLNode* copied_node = dom_tree_copy(other.node.get());
            if (copied_node) temp.reset(copied_node);
        }

        /* commit changes */
        node = std::move(temp);
        minify = other.minify;
    }

    return *this;
}

HtmlParser& HtmlParser::operator =(HtmlParser&& other) noexcept {
    if (this != &other) {
        node = std::move(other.node);
        minify = other.minify;
        other.minify = 0;
    }

    return *this;
}

std::ostream& operator <<(std::ostream& out, const HtmlParser& parser) {
    std::string output = parser.toString();
    out << output;
    return out;
}

void HtmlParser::setParent(std::unique_ptr<HTMLNode, decltype(&html2tex_free_node)> new_node) {
    node = std::move(new_node);
}

std::istream& operator >>(std::istream& in, HtmlParser& parser) {
    /* check if stream is in good state first */
    if (!in.good()) {
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(nullptr, &html2tex_free_node));
        return in;
    }

    std::ostringstream stream;
    char buffer[4096];

    /* read stream in chunks until EOF or failure */
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
        stream.write(buffer, in.gcount());
        if (in.eof()) break;
    }

    /* check if we actually read anything */
    std::string html_content = stream.str();

    if (html_content.empty()) {
        /* clear failure state for future reads */
        if (in.fail() && !in.eof()) in.clear();

        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(nullptr, &html2tex_free_node));
        return in;
    }

    /* only check EOF state after ensuring that content exists */
    if (!in.eof() && in.fail()) in.clear();

    HTMLNode* raw_node = parser.minify ?
        html2tex_parse_minified(html_content.c_str()) :
        html2tex_parse(html_content.c_str());

    if (raw_node) {
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(raw_node, &html2tex_free_node));
    }
    else {
        /* reset to empty state if parsing fails */
        parser.setParent(std::unique_ptr<HTMLNode,
            decltype(&html2tex_free_node)>(nullptr, &html2tex_free_node));
    }

    return in;
}

HtmlParser HtmlParser::FromStream(std::ifstream& input) {
    /* fast fail checks */
    if (!input.is_open() || input.bad())
        return HtmlParser();

    /* get file size for optimal buffer sizing */
    const std::streampos current_pos = input.tellg();
    input.seekg(0, std::ios::end);

    const std::streamsize file_size = input.tellg() - current_pos;
    input.seekg(current_pos);

    /* optimize buffer size based on file size */
    constexpr size_t SMALL_FILE = 65'536;

    constexpr size_t MEDIUM_FILE = 1'048'576;
    constexpr size_t MAX_READ = 134'217'728;

    /* reject files over limit immediately */
    if (file_size > static_cast<std::streamsize>(MAX_READ))
        return HtmlParser();

    /* choose optimal buffer size */
    size_t buffer_size;

    if (file_size <= static_cast<std::streamsize>(SMALL_FILE))
        buffer_size = 4096;
    else if (file_size <= static_cast<std::streamsize>(MEDIUM_FILE))
        buffer_size = 16384;
    else
        buffer_size = 65536;

    /* optimize initial capacity */
    std::string content;

    if (file_size > 0) {
        const size_t reserve_size = static_cast<size_t>(file_size) + 1;
        if (reserve_size <= MAX_READ) content.reserve(reserve_size);
    }

    /* use vector for potentially better memory handling */
    std::vector<char> buffer(buffer_size);
    size_t total_read = 0;
    bool read_error = false;

    /* single-pass optimized read loop */
    while (total_read < MAX_READ) {
        /* read data directly */
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytes_read = input.gcount();

        /* handle read results efficiently */
        if (bytes_read <= 0) {
            if (input.eof()) break;

            if (input.fail() || input.bad()) {
                read_error = true;
                break;
            }
            
            /* non-blocking or empty */
            break;
        }

        /* convert safely with single check */
        const size_t bytes_size = static_cast<size_t>(bytes_read);

        /* check for size limit violation */
        if (bytes_size > MAX_READ - total_read) {
            /* append only what fits */
            const size_t can_append = MAX_READ - total_read;

            if (can_append > 0 && can_append <= buffer.size())
                content.append(buffer.data(), can_append);
            
            break;
        }

        /* bulk append for performance */
        content.append(buffer.data(), bytes_size);
        total_read += bytes_size;

        /* early exit if EOF reached */
        if (input.eof() || bytes_size < buffer.size())
            break;
    }

    /* validate read operation */
    if (read_error || content.empty() || input.bad()) {
        input.clear();

        if (current_pos != std::streampos(-1))
            input.seekg(current_pos);

        return HtmlParser();
    }

    /* parse final content */
    if (!content.empty()) {
        HTMLNode* raw_node = html2tex_parse(content.c_str());
        if (raw_node) return HtmlParser(raw_node);
    }

    return HtmlParser();
}

HtmlParser HtmlParser::FromHtml(const std::string& filePath) {
    /* check if stream is usable */
    std::ifstream fin(filePath, std::ios::binary);

    if (!fin.is_open() || fin.bad())
        return HtmlParser();

    /* save original position */
    std::streampos original_pos = fin.tellg();

    /* try to read with reasonable limit */
    const size_t MAX_READ = 134'217'728;

    std::string content;
    content.reserve(65536);

    char buffer[4096];
    size_t total_read = 0;

    /* read chunks until EOF or limit reached */
    while (total_read < MAX_READ && fin.good()) {
        fin.read(buffer, sizeof(buffer));
        std::streamsize bytes = fin.gcount();

        if (bytes <= 0)
            break;

        /* check for overflow */
        size_t bytes_size_t = static_cast<size_t>(bytes);
        size_t remaining_limit = MAX_READ - total_read;

        if (bytes_size_t > remaining_limit) {
            /* would exceed limit - read partial */
            if (remaining_limit > 0 && remaining_limit <= sizeof(buffer))
                content.append(buffer, remaining_limit);

            break;
        }

        content.append(buffer, bytes_size_t);
        total_read += bytes_size_t;
        if (fin.eof()) break;
    }

    /* restore position and return empty */
    if (content.empty() || fin.bad()) {
        fin.clear(); 
        fin.seekg(original_pos);

        if(fin.is_open()) fin.close();
        return HtmlParser();
    }

    /* clear any failure flags (except EOF) */
    if (!fin.eof()) fin.clear();

    /* parse the content */
    if (!content.empty()) {
        HTMLNode* raw_node = html2tex_parse(content.c_str());

        if (raw_node) {
            if (fin.is_open()) fin.close();
            return HtmlParser(raw_node);
        }
    }

    if (fin.is_open()) fin.close();
    return HtmlParser();
}

std::string HtmlParser::toString() const {
    if (!node) return "";
    char* output = get_pretty_html(node.get());

    /* additional safety check */
    if (!output) return "";
    std::string result(output);

    free(output);
    return result;
}