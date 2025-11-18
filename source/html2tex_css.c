#include "html2tex.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <math.h>

#define HT_MAX_CSS_PROPERTIES 50

/* helper function to remove !important and trim whitespace */
static char* clean_css_value(char* value) {
    if (!value) return NULL;

    char* cleaned = strdup(value);
    if (!cleaned) return NULL;

    /* remove !important */
    char* important_pos = strstr(cleaned, "!important");
    if (important_pos) *important_pos = '\0';

    /* trim trailing whitespace */
    char* end = cleaned + strlen(cleaned) - 1;

    while (end > cleaned && isspace(*end))
        *end-- = '\0';

    /* trim leading whitespace */
    char* start = cleaned;

    while (*start && isspace(*start))
        start++;

    if (start != cleaned)
        memmove(cleaned, start, strlen(start) + 1);

    return cleaned;
}

/* check if element is block-level */
int is_block_element(const char* tag_name) {
    if (!tag_name) return 0;

    const char* block_tags[] = {
        "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
        "ul", "ol", "li", "table", "tr", "td", "th",
        "blockquote", "section", "article", "header", "footer",
        "nav", "aside", "main", "figure", "figcaption", NULL
    };

    for (int i = 0; block_tags[i]; i++) {
        if (strcmp(tag_name, block_tags[i]) == 0)
            return 1;
    }

    return 0;
}

/* check if element is inline */
int is_inline_element(const char* tag_name) {
    if (!tag_name) return 0;

    const char* inline_tags[] = {
        "span", "a", "strong", "em", "b", "i", "u", "code",
        "font", "mark", "small", "sub", "sup", "time", NULL
    };

    for (int i = 0; inline_tags[i]; i++) {
        if (strcmp(tag_name, inline_tags[i]) == 0)
            return 1;
    }

    return 0;
}

/* check if CSS value contains !important */
static int has_important(const char* value) {
    if (!value) return 0;
    return (strstr(value, "!important") != NULL);
}

/* parse CSS style string into structured properties with !important support */
CSSProperties* parse_css_style(const char* style_str) {
    if (!style_str) return NULL;
    CSSProperties* props = calloc(1, sizeof(CSSProperties));

    if (!props) return NULL;
    char* copy = strdup(style_str);

    if (!copy) {
        free(props);
        return NULL;
    }

    char* token = strtok(copy, ";");

    while (token) {
        /* trim whitespace */
        while (*token == ' ') token++;
        char* colon = strchr(token, ':');

        if (colon) {
            *colon = '\0';
            char* property = token;
            char* value = colon + 1;

            /* trim value and remove !important */
            char* cleaned_value = clean_css_value(value);
            if (!cleaned_value) {
                token = strtok(NULL, ";");
                continue;
            }

            /* map CSS properties */
            if (strcmp(property, "font-weight") == 0)
                props->font_weight = strdup(cleaned_value);
            else if (strcmp(property, "font-style") == 0)
                props->font_style = strdup(cleaned_value);
            else if (strcmp(property, "font-family") == 0)
                props->font_family = strdup(cleaned_value);
            else if (strcmp(property, "font-size") == 0)
                props->font_size = strdup(cleaned_value);
            else if (strcmp(property, "color") == 0)
                props->color = strdup(cleaned_value);
            else if (strcmp(property, "background-color") == 0)
                props->background_color = strdup(cleaned_value);
            else if (strcmp(property, "text-align") == 0)
                props->text_align = strdup(cleaned_value);
            else if (strcmp(property, "text-decoration") == 0)
                props->text_decoration = strdup(cleaned_value);
            else if (strcmp(property, "margin-top") == 0)
                props->margin_top = strdup(cleaned_value);
            else if (strcmp(property, "margin-bottom") == 0)
                props->margin_bottom = strdup(cleaned_value);
            else if (strcmp(property, "margin-left") == 0)
                props->margin_left = strdup(cleaned_value);
            else if (strcmp(property, "margin-right") == 0)
                props->margin_right = strdup(cleaned_value);
            else if (strcmp(property, "padding-top") == 0)
                props->padding_top = strdup(cleaned_value);
            else if (strcmp(property, "padding-bottom") == 0)
                props->padding_bottom = strdup(cleaned_value);
            else if (strcmp(property, "padding-left") == 0)
                props->padding_left = strdup(cleaned_value);
            else if (strcmp(property, "padding-right") == 0)
                props->padding_right = strdup(cleaned_value);
            else if (strcmp(property, "width") == 0)
                props->width = strdup(cleaned_value);
            else if (strcmp(property, "height") == 0)
                props->height = strdup(cleaned_value);
            else if (strcmp(property, "border") == 0)
                props->border = strdup(cleaned_value);
            else if (strcmp(property, "border-color") == 0)
                props->border_color = strdup(cleaned_value);
            else if (strcmp(property, "display") == 0)
                props->display = strdup(cleaned_value);
            else if (strcmp(property, "float") == 0)
                props->float_pos = strdup(cleaned_value);
            else if (strcmp(property, "vertical-align") == 0)
                props->vertical_align = strdup(cleaned_value);

            free(cleaned_value);
        }

        token = strtok(NULL, ";");
    }

    free(copy);
    return props;
}

/* convert CSS length to LaTeX points */
int css_length_to_pt(const char* length_str) {
    if (!length_str) return 0;

    double value;
    char unit[10] = "";

    if (sscanf(length_str, "%lf%s", &value, unit) < 1)
        return 0;

    /* convert to points */
    if (strcmp(unit, "px") == 0) {
        /* assuming 96px = 1inch = 72pt */
        return (int)(value * 72.0 / 96.0);
    }
    else if (strcmp(unit, "pt") == 0)
        return (int)value;
    else if (strcmp(unit, "em") == 0) {
        /* 1em ≈ 10pt in LaTeX */
        return (int)(value * 10.0);
    }
    else if (strcmp(unit, "rem") == 0)
        return (int)(value * 10.0);
    else if (strcmp(unit, "%") == 0) {
        /* percentage of text width - handle differently */
        return (int)(value * 0.01 * 400); /* approximation */
    }
    else if (strcmp(unit, "cm") == 0)
        return (int)(value * 28.346);
    else if (strcmp(unit, "mm") == 0)
        return (int)(value * 2.8346);
    else if (strcmp(unit, "in") == 0)
        return (int)(value * 72.0);

    /* default assumption */
    return (int)value;
}

/* convert CSS color to hex format */
char* css_color_to_hex(const char* color_value) {
    if (!color_value) return NULL;

    char* result = NULL;

    if (color_value[0] == '#') {
        /* hex color */
        if (strlen(color_value) == 4) { 
            /* #RGB format */
            result = malloc(7);

            snprintf(result, 7, "%c%c%c%c%c%c",
                color_value[1], color_value[1],
                color_value[2], color_value[2],
                color_value[3], color_value[3]);
        }
        else { 
            /* #RRGGBB format */
            result = strdup(color_value + 1);
        }
    }
    else if (strncmp(color_value, "rgb(", 4) == 0) {
        /* RGB color */
        int r, g, b;

        if (sscanf(color_value, "rgb(%d, %d, %d)", &r, &g, &b) == 3) {
            result = malloc(7);
            snprintf(result, 7, "%02X%02X%02X", r, g, b);
        }
    }
    else if (strncmp(color_value, "rgba(", 5) == 0) {
        /* RGBA color - ignore alpha */
        int r, g, b;
        float a;

        if (sscanf(color_value, "rgba(%d, %d, %d, %f)", &r, &g, &b, &a) == 4) {
            result = malloc(7);
            snprintf(result, 7, "%02X%02X%02X", r, g, b);
        }
    }
    else {
        /* Named colors */
        struct {
            const char* name;
            const char* hex;
        } color_map[] = {
            {"black", "000000"}, {"white", "FFFFFF"},
            {"red", "FF0000"}, {"green", "008000"},
            {"blue", "0000FF"}, {"yellow", "FFFF00"},
            {"cyan", "00FFFF"}, {"magenta", "FF00FF"},
            {"gray", "808080"}, {"grey", "808080"},
            {"silver", "C0C0C0"}, {"maroon", "800000"},
            {"olive", "808000"}, {"lime", "00FF00"},
            {"aqua", "00FFFF"}, {"teal", "008080"},
            {"navy", "000080"}, {"fuchsia", "FF00FF"},
            {"purple", "800080"}, {"orange", "FFA500"},
            {NULL, NULL}
        };

        for (int i = 0; color_map[i].name; i++) {
            if (strcasecmp(color_value, color_map[i].name) == 0) {
                result = strdup(color_map[i].hex);
                break;
            }
        }

        if (!result) {
            /* default to black for unknown colors */
            result = strdup("000000");
        }
    }

    if (result) {
        /* convert to uppercase */
        for (char* p = result; *p; p++)
            *p = toupper(*p);
    }

    return result;
}

/* apply CSS properties to LaTeX converter */
void apply_css_properties(LaTeXConverter* converter, CSSProperties* props, const char* tag_name) {
    if (!converter || !props) return;
    int is_block = is_block_element(tag_name);
    int is_inline = is_inline_element(tag_name);

    /* track applied environments for proper closing */
    converter->state.css_environments = 0;

    /* text alignment (block elements only) */
    if (is_block && props->text_align) {
        if (strcmp(props->text_align, "center") == 0) {
            /* center environment */
            append_string(converter, "\\begin{center}\n");
            converter->state.css_environments |= 1;
        }
        else if (strcmp(props->text_align, "right") == 0) {
            /* flushright environment */
            append_string(converter, "\\begin{flushright}\n");
            converter->state.css_environments |= 2;
        }
        else if (strcmp(props->text_align, "left") == 0) {
            /* flushleft environment */
            append_string(converter, "\\begin{flushleft}\n");
            converter->state.css_environments |= 4;
        }
        else if (strcmp(props->text_align, "justify") == 0) {
            /* justifying command */
            append_string(converter, "\\justifying\n");
            converter->state.css_environments |= 8;
        }
    }

    /* margins (block elements) */
    if (is_block) {
        if (props->margin_top) {
            int pt = css_length_to_pt(props->margin_top);
            if (pt > 0) {
                char margin_cmd[32];
                snprintf(margin_cmd, sizeof(margin_cmd), "\\vspace*{%dpt}\n", pt);
                append_string(converter, margin_cmd);
            }
        }

        if (props->margin_bottom) {
            int pt = css_length_to_pt(props->margin_bottom);
            if (pt > 0) converter->state.pending_margin_bottom = pt;
        }
    }

    /* background color */
    if (props->background_color) {
        char* hex_color = css_color_to_hex(props->background_color);

        if (hex_color) {
            append_string(converter, "\\colorbox[HTML]{");
            append_string(converter, hex_color);
            append_string(converter, "}{");

            free(hex_color);
            converter->state.css_braces++;
        }
    }

    /* text color */
    if (props->color) {
        char* hex_color = css_color_to_hex(props->color);
        if (hex_color) {
            append_string(converter, "\\textcolor[HTML]{");
            append_string(converter, hex_color);
            append_string(converter, "}{");

            free(hex_color);
            converter->state.css_braces++;
        }
    }

    /* font weight */
    if (props->font_weight) {
        if (strcmp(props->font_weight, "bold") == 0 ||
            strcmp(props->font_weight, "bolder") == 0 ||
            atoi(props->font_weight) >= 600) {
            append_string(converter, "\\textbf{");
            converter->state.css_braces++;
        }
    }

    /* font style */
    if (props->font_style) {
        if (strcmp(props->font_style, "italic") == 0) {
            append_string(converter, "\\textit{");
            converter->state.css_braces++;
        }
        else if (strcmp(props->font_style, "oblique") == 0) {
            append_string(converter, "\\textsl{");
            converter->state.css_braces++;
        }
    }

    /* font family */
    if (props->font_family) {
        if (strstr(props->font_family, "monospace") ||
            strstr(props->font_family, "Courier")) {
            append_string(converter, "\\texttt{");
            converter->state.css_braces++;
        }
        else if (strstr(props->font_family, "sans") ||
            strstr(props->font_family, "Arial") ||
            strstr(props->font_family, "Helvetica")) {
            append_string(converter, "\\textsf{");
            converter->state.css_braces++;
        }
        else if (strstr(props->font_family, "serif") ||
            strstr(props->font_family, "Times")) {
            append_string(converter, "\\textrm{");
            converter->state.css_braces++;
        }
    }

    /* text decoration */
    if (props->text_decoration) {
        if (strstr(props->text_decoration, "underline")) {
            append_string(converter, "\\underline{");
            converter->state.css_braces++;
        }
        if (strstr(props->text_decoration, "line-through")) {
            append_string(converter, "\\sout{");
            converter->state.css_braces++;
        }
    }

    /* font size */
    if (props->font_size) {
        int pt = css_length_to_pt(props->font_size);
        if (pt > 0) {
            if (pt <= 8)
                append_string(converter, "\\tiny{");
            else if (pt <= 10)
                append_string(converter, "\\small{");
            else if (pt <= 12)
                append_string(converter, "\\normalsize{");
            else if (pt <= 14)
                append_string(converter, "\\large{");
            else if (pt <= 18)
                append_string(converter, "\\Large{");
            else if (pt <= 24)
                append_string(converter, "\\LARGE{");
            else
                append_string(converter, "\\huge{");

            converter->state.css_braces++;
        }
    }
}

/* free CSS properties */
void free_css_properties(CSSProperties* props) {
    if (!props) return;
    free(props->font_weight);

    free(props->font_style);
    free(props->font_family);

    free(props->font_size);
    free(props->color);

    free(props->background_color);
    free(props->text_align);

    free(props->text_decoration);
    free(props->margin_top);

    free(props->margin_bottom);
    free(props->margin_left);

    free(props->margin_right);
    free(props->padding_top);

    free(props->padding_bottom);
    free(props->padding_left);

    free(props->padding_right);
    free(props->width);

    free(props->height);
    free(props->border);

    free(props->border_color);
    free(props->display);

    free(props->float_pos);
    free(props->vertical_align);
    free(props);
}