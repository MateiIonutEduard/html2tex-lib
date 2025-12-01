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
		int figure_counter;
		
		int table_id_counter;
		int figure_id_counter;
		
		int image_id_counter;
		int image_caption_counter;
		
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
        
        /* track applied CSS properties */
        int has_bold;
        int has_italic;
		
        int has_underline;
        int has_color;
		
        int has_background;
        int has_font_family;
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
		
		char* image_output_dir;
        int download_images;
		int image_counter;
    };

    /* Creates a new LaTeXConverter* and allocates memory. */
    LaTeXConverter* html2tex_create(void);
	
	/* Returns a copy of the LaTeXConverter* object. */
	LaTeXConverter* html2tex_copy(LaTeXConverter*);
	
	/* Frees a LaTeXConverter* structure. */
    void html2tex_destroy(LaTeXConverter* converter);
    
	/* Parses input HTML and converts it to LaTeX. */
    char* html2tex_convert(LaTeXConverter* converter, const char* html);
	
	/* Returns the error code from the HTML-to-LaTeX conversion. */
    int html2tex_get_error(const LaTeXConverter* converter);
	
	/* Returns the error message from the HTML-to-LaTeX conversion. */
    const char* html2tex_get_error_message(const LaTeXConverter* converter);
    
	/* Writes the input string in LaTeX format. */
    void append_string(LaTeXConverter* converter, const char* str);
	
	/* Recursively converts a DOM child node to LaTeX. */
    void convert_children(LaTeXConverter* converter, HTMLNode* node);

    /* Parse the virtual DOM tree without optimizations. */
    HTMLNode* html2tex_parse(const char* html);
	
	/* Parse the virtual DOM tree with minification for improved performance. */
	HTMLNode* html2tex_parse_minified(const char* html);
	
	/* Creates a new instance from the input DOM tree. */
	HTMLNode* dom_tree_copy(HTMLNode* node);
	
	/* Frees the memory for the HTMLNode* instance. */
    void html2tex_free_node(HTMLNode* node);
	
	/* Sets the download output directory. */
    void html2tex_set_image_directory(LaTeXConverter* converter, const char* dir);
	
	/* Toggles image downloading according to the enable flag. */
    void html2tex_set_download_images(LaTeXConverter* converter, int enable);
	
	/* Downloads an image from the specified URL. */
	char* download_image_src(const char* src, const char* output_dir, int image_counter);
	
	/* Returns whether src contains a base64-encoded image. */
	int is_base64_image(const char* src);
	
	/* Initializes download processing. */
	int image_utils_init(void);
	
	/* Frees image-download resources. */
	void image_utils_cleanup(void);
	
	/* Parses inline CSS from style. */
	CSSProperties* parse_css_style(const char* style_str);
	
	/* Releases memory used by CSSProperties for managing styles. */
	void free_css_properties(CSSProperties* props);

	/* Applies inline CSS to the specified HTML element. */
	void apply_css_properties(LaTeXConverter* converter, CSSProperties* props, const char* tag_name);
	
	/* Finalizes inline CSS and frees resources. */
	void end_css_properties(LaTeXConverter* converter, CSSProperties* props, const char* tag_name);

	/* Return the DOM tree after minification. */
	HTMLNode* html2tex_minify_html(HTMLNode* root);
	
	/* Writes formatted HTML to the output file. */
	int write_pretty_html(HTMLNode* root, const char* filename);
	
	/* Returns the HTML as a formatted string. */
	char* get_pretty_html(HTMLNode* root);

	/* Converts a CSS length to LaTeX points. */
	int css_length_to_pt(const char* length_str);
	
	/* Converts a CSS color to hexadecimal format. */
	char* css_color_to_hex(const char* color_value);
	
	/* Checks if an element is block-level. */
	int is_block_element(const char* tag_name);
	
	/* Checks if an element is inline. */
	int is_inline_element(const char* tag_name);
	
	/* Resets the CSS style engine state for the virtual DOM. */
	void reset_css_state(LaTeXConverter* converter);
	
	/* Returns a null-terminated duplicate of the string referenced by str. */
    char* portable_strdup(const char* str);
	
	/* Convert an integer to a null-terminated string using the given radix and store it in buffer. */
	void portable_itoa(int value, char* buffer, int radix);
	
	#ifdef _MSC_VER
	#define strdup portable_strdup
	#define html2tex_itoa(value, buffer, radix) _itoa((value), (buffer), (radix))
	#else
	#define html2tex_itoa(value, buffer, radix) portable_itoa((value), (buffer), (radix))
	#endif
	
	#ifdef _WIN32
    #define mkdir(path) _mkdir(path)
	#else
	#define mkdir(path) mkdir(path, 0755)
	#endif

#ifdef __cplusplus
}
#endif

#endif