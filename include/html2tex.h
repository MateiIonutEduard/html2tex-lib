#ifndef HTML2TEX_H
#define HTML2TEX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct HTMLNode HTMLNode;
    typedef struct HTMLAttribute HTMLAttribute;
	
	typedef struct ConverterState ConverterState;
    typedef struct LaTeXConverter LaTeXConverter;
	typedef struct CSSProperties CSSProperties;

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
    struct ConverterState {
        int indent_level;
        int list_level;
		
        int in_paragraph;
        int in_list;
		
        int table_counter;
        int in_table;
		
        int in_table_row;
        int in_table_cell;
		
        int table_columns;
        int current_column;
		char* table_caption;
		
		/* CSS conversion state */
		int css_braces;
		
		int css_environments;
		int pending_margin_bottom;
    };
	
	struct CSSProperties {
		char* font_weight;
		char* font_style;
		
		char* font_family;
		char* font_size;
		
		char* color;
		char* background_color;
		
		char* text_align;
		char* text_decoration;
		
		char* margin_top;
		char* margin_bottom;
		
		char* margin_left;
		char* margin_right;
		
		char* padding_top;
		char* padding_bottom;
		
		char* padding_left;
		char* padding_right;
		
		char* width;
		char* height;
		
		char* border;
		char* border_color;
		char* display;
		
		char* float_pos;
		char* vertical_align;
	};

    /* main converter structure */
    struct LaTeXConverter {
        char* output;
        size_t output_size;
		
        size_t output_capacity;
        ConverterState state;
		
        int error_code;
        char error_message[256];
    };

    /* core API functions */
    LaTeXConverter* html2tex_create(void);
    void html2tex_destroy(LaTeXConverter* converter);

    char* html2tex_convert(LaTeXConverter* converter, const char* html);
    int html2tex_get_error(const LaTeXConverter* converter);
    const char* html2tex_get_error_message(const LaTeXConverter* converter);
    
    void append_string(LaTeXConverter* converter, const char* str);
    void convert_children(LaTeXConverter* converter, HTMLNode* node);

    /* utility functions */
    HTMLNode* html2tex_parse(const char* html);
    void html2tex_free_node(HTMLNode* node);
	
	/* CSS parsing functions */
	CSSProperties* parse_css_style(const char* style_str);
	void free_css_properties(CSSProperties* props);

	/* CSS to LaTeX conversion functions */
	void apply_css_properties(LaTeXConverter* converter, CSSProperties* props, const char* tag_name);
	void end_css_properties(LaTeXConverter* converter, CSSProperties* props, const char* tag_name);

	/* utility functions */
	int css_length_to_pt(const char* length_str);
	char* css_color_to_hex(const char* color_value);
	
	int is_block_element(const char* tag_name);
	int is_inline_element(const char* tag_name);

#ifdef __cplusplus
}
#endif

#endif