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
    /* early exit for bad streams */
    if (in.bad()) {
        parser.setParent({ nullptr, &html2tex_free_node });
        return in;
    }

    /* use sentry for proper stream behavior */
    std::istream::sentry sentry(in);

    if (!sentry) {
        parser.setParent({ nullptr, &html2tex_free_node });
        return in;
    }

    constexpr std::size_t kMaxSize = 134'217'728;
    std::string content;

    /* use rdbuf for optimal reading */
    auto* sbuf = in.rdbuf();

    if (sbuf) {
        /* get stream size hint if available */
        const std::streamsize available = sbuf->in_avail();

        if (available > 0) {
            const std::size_t hint_size = static_cast<std::size_t>(
                std::min<std::streamsize>(available, kMaxSize));
            content.reserve(std::min(hint_size, std::size_t(262'144)));
        }
    }

    /* read directly using streambuf */
    bool success = false;
    std::size_t total_read = 0;

    if (sbuf) {
        std::streambuf::int_type ch;
        content.clear();

        while (total_read < kMaxSize) {
            ch = sbuf->sbumpc();

            if (ch == std::streambuf::traits_type::eof()) {
                success = true;
                break;
            }

            content.push_back(std::streambuf::traits_type::to_char_type(ch));
            ++total_read;

            /* bulk read optimization */
            if (total_read == 1) {
                const std::streamsize remaining = sbuf->in_avail();

                if (remaining > 0) {
                    const std::size_t to_read = std::min(
                        static_cast<std::size_t>(remaining),
                        kMaxSize - total_read);

                    if (to_read > 0) {
                        content.resize(total_read + to_read);
                        const std::streamsize actually_read = sbuf->sgetn(
                            &content[total_read], static_cast<std::streamsize>(to_read));

                        total_read += static_cast<std::size_t>(actually_read);
                        content.resize(total_read);

                        /* EOF reached */
                        if (actually_read < static_cast<std::streamsize>(to_read)) {
                            success = true;
                            break;
                        }
                    }
                }
            }
        }

        /* size limit reached */
        if (total_read >= kMaxSize)
            in.clear(in.rdstate() & ~std::ios_base::failbit);
    }

    /* fallback to standard read if streambuf method failed */
    if (!success) {
        in.clear();
        return operator>>(in, parser);
    }

    /* parse the content */
    if (!content.empty()) {
        HTMLNode* raw_node = parser.minify ?
            html2tex_parse_minified(content.c_str()) :
            html2tex_parse(content.c_str());

        if (raw_node) {
            parser.setParent({ raw_node, &html2tex_free_node });
            return in;
        }
    }

    parser.setParent({ nullptr, &html2tex_free_node });
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
    /* open with optimal flags for reading */
    std::ifstream fin(filePath, std::ios::binary | std::ios::ate);
    if (!fin.is_open()) return HtmlParser();

    /* get exact file size */
    const std::streamsize file_size = fin.tellg();
    fin.seekg(0, std::ios::beg);

    /* reject invalid or empty files */
    if (file_size <= 0) {
        fin.close();

        /* truly empty file */
        if (file_size == 0) {
            /* parse empty content */
            HTMLNode* raw_node = html2tex_parse("");
            if (raw_node) return HtmlParser(raw_node);
        }

        return HtmlParser();
    }

    /* enforce absolute size limit for security reasons */
    constexpr size_t MAX_READ = 134'217'728;

    /* file too large */
    if (static_cast<size_t>(file_size) > MAX_READ) {
        fin.close();
        return HtmlParser();
    }

    /* single allocation for entire file, good for performance */
    std::string content;
    content.resize(static_cast<size_t>(file_size));

    /* read entire file in one operation, that is fastest for known size */
    fin.read(&content[0], file_size);
    const bool read_ok = fin.gcount() == file_size && !fin.bad();

    /* close immediately after read */
    fin.close();

    /* validate complete read */
    if (!read_ok) return HtmlParser();

    /* parse the content */
    HTMLNode* raw_node = html2tex_parse(content.c_str());

    if (raw_node) return HtmlParser(raw_node);
    return HtmlParser();
}

HTMLNode* HtmlParser::GetHtmlNode() const noexcept { 
    return node.get(); 
}

bool HtmlParser::HasContent() const noexcept { 
    return node != nullptr; 
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