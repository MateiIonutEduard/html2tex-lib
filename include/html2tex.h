#ifndef HTML2TEX_H
#define HTML2TEX_H

#include <stddef.h>

#ifdef __cplusplus

extern "C" {
#endif

    typedef struct HTMLNode HTMLNode;
    typedef struct HTMLAttribute HTMLAttribute;
    typedef struct LaTeXConverter LaTeXConverter;

    /* HTML node structure */
    struct HTMLNode {
        char* tag;
        char* content;
        HTMLAttribute* attributes;
        HTMLNode* children;
        HTMLNode* next;
        HTMLNode* parent;
    };

    /* HTML attribute structure */
    struct HTMLAttribute {
        char* key;
        char* value;
        HTMLAttribute* next;
    };

    /* converter configuration */
    typedef struct {
        int indent_level;
        int list_level;
        int in_paragraph;
        int in_list;
        int table_counter;
    } ConverterState;

    /* Main converter structure */
    struct LaTeXConverter {
        char* output;
        size_t output_size;
        size_t output_capacity;
        ConverterState state;
        int error_code;
        char error_message[256];
    };

    /* Core API functions */
    LaTeXConverter* html2tex_create(void);
    void html2tex_destroy(LaTeXConverter* converter);

    char* html2tex_convert(LaTeXConverter* converter, const char* html);
    int html2tex_get_error(const LaTeXConverter* converter);
    const char* html2tex_get_error_message(const LaTeXConverter* converter);
	
	void append_string(LaTeXConverter* converter, const char* str);
	void convert_children(LaTeXConverter* converter, HTMLNode* node);

    /* Utility functions */
    HTMLNode* html2tex_parse(const char* html);
    void html2tex_free_node(HTMLNode* node);

#ifdef __cplusplus
}
#endif

#endif