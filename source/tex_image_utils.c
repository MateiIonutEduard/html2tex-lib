#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
#define _WINSOCKAPI_
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <winsock2.h>

#include <sys/types.h>
#include <sys/stat.h>

#define stat _stat
#define fstat _fstat
#define fileno _fileno
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include <curl/curl.h>
#include "html2tex.h"

/* Base64 decoding table */
static const unsigned char base64_table[256] = {
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x3E, 0x80, 0x80, 0x80, 0x3F,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
    0x80, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

/* Check if string is base64 encoded image */
int is_base64_image(const char* src) {
    if (!src) return 0;
    return (strncmp(src, "data:image/", 11) == 0);
}

/* Extract MIME type from base64 data URI. */
static char* extract_mime_type(const char* base64_data) {
    if (!base64_data) return NULL;

    const char* prefix = "data:";
    const char* semicolon = strchr(base64_data, ';');
    if (!semicolon) return NULL;

    size_t len = semicolon - (base64_data + strlen(prefix));
    if (len == 0) return NULL;

    char* mime_type = malloc(len + 1);
    if (!mime_type) return NULL;

    strncpy(mime_type, base64_data + strlen(prefix), len);
    mime_type[len] = '\0';

    return mime_type;
}

/* Extract base64 data from data URI. */
static char* extract_base64_data(const char* base64_data) {
    if (!base64_data) return NULL;
    const char* base64_prefix = "base64,";

    const char* data_start = strstr(base64_data, base64_prefix);
    if (!data_start) return NULL;
    data_start += strlen(base64_prefix);

    /* remove any whitespace from base64 data */
    size_t len = strlen(data_start);
    char* clean_data = malloc(len + 1);

    if (!clean_data) return NULL;
    char* dest = clean_data;

    for (const char* src = data_start; *src; src++)
        if (!isspace(*src))
            *dest++ = *src;

    *dest = '\0';

    return clean_data;
}

/* Get file extension from MIME type. */
static const char* get_extension_from_mime_type(const char* mime_type) {
    if (!mime_type) return ".bin";

    if (strstr(mime_type, "jpeg") || strstr(mime_type, "jpg")) return ".jpg";
    else if (strstr(mime_type, "png")) return ".png";
    else if (strstr(mime_type, "gif")) return ".gif";
    else if (strstr(mime_type, "bmp")) return ".bmp";
    else if (strstr(mime_type, "tiff")) return ".tiff";
    else if (strstr(mime_type, "webp")) return ".webp";
    else if (strstr(mime_type, "svg")) return ".svg";
    else return ".bin";
}

/* Robust base64 decoding function. */
static unsigned char* base64_decode(const char* data, size_t input_len, size_t* output_len) {
    if (!data || input_len == 0) return NULL;

    /* invalid base64 string */
    if (input_len % 4 != 0) return NULL;
    *output_len = (input_len * 3) / 4;

    if (data[input_len - 1] == '=') (*output_len)--;
    if (data[input_len - 2] == '=') (*output_len)--;

    unsigned char* decoded = malloc(*output_len);
    if (!decoded) return NULL;

    const unsigned char* input = (const unsigned char*)data;
    unsigned char* output = decoded;

    for (size_t i = 0; i < input_len; ) {
        unsigned long int sextet_a = input[i] == '=' ? 0 & i++ : base64_table[input[i++]];
        unsigned long int sextet_b = input[i] == '=' ? 0 & i++ : base64_table[input[i++]];

        unsigned long int sextet_c = input[i] == '=' ? 0 & i++ : base64_table[input[i++]];
        unsigned long int sextet_d = input[i] == '=' ? 0 & i++ : base64_table[input[i++]];

        if (sextet_a == 0x80 || sextet_b == 0x80 || sextet_c == 0x80 || sextet_d == 0x80) {
            free(decoded);
            return NULL;
        }

        unsigned long int triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (output - decoded < (ptrdiff_t)(*output_len - 2))
            *output++ = (triple >> 2 * 8) & 0xFF;

        if (output - decoded < (ptrdiff_t)(*output_len - 1))
            *output++ = (triple >> 1 * 8) & 0xFF;

        if (output - decoded < (ptrdiff_t)(*output_len))
            *output++ = (triple >> 0 * 8) & 0xFF;
    }

    return decoded;
}

/* Decode base64 data and save to file. */
static int save_base64_image(const char* base64_data, const char* filename) {
    if (!base64_data || !filename) return 0;
    char* clean_data = extract_base64_data(base64_data);

    if (!clean_data) return 0;
    size_t input_len = strlen(clean_data);

    if (input_len == 0) {
        free(clean_data);
        return 0;
    }

    size_t output_len = 0;
    unsigned char* decoded_data = base64_decode(clean_data, input_len, &output_len);
    free(clean_data);

    if (!decoded_data || output_len == 0)
        return 0;

    /* write to file */
    FILE* file = fopen(filename, "wb");

    if (!file) {
        free(decoded_data);
        return 0;
    }

    size_t written = fwrite(decoded_data, 1, output_len, file);
    fclose(file);

    free(decoded_data);
    return (written == output_len) ? 1 : 0;
}

/* libcurl write callback */
static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

/* Download image from URL using libcurl. */
static int download_image_url(const char* url, const char* filename) {
    if (!url || !filename) return 0;
    CURL* curl = NULL;

    FILE* fp = NULL;
    CURLcode res;

    int success = 0;
    curl = curl_easy_init();

    if (!curl) return 0;
    fp = fopen(filename, "wb");

    if (!fp) {
        curl_easy_cleanup(curl);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "html2tex/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) success = 1;
    }

    if (fp) fclose(fp);
    if (curl) curl_easy_cleanup(curl);
    return success;
}

/* Create directory if it doesn't exist. */
static int create_directory_if_not_exists(const char* dir_path) {
    if (!dir_path) return -1;
    struct stat st = { 0 };

    if (stat(dir_path, &st) == -1) {
        /* create directory recursively */
        char* path_copy = strdup(dir_path);

        if (!path_copy) return -1;
        char* p = path_copy;

#ifdef _WIN32
        if (strlen(p) > 2 && p[1] == ':')
            p += 2;
#endif

        while (*p) {
            if (*p == '/' || *p == '\\') {
                char old_char = *p;
                *p = '\0';

                if (stat(path_copy, &st) == -1) {
                    if (mkdir(path_copy) != 0) {
                        free(path_copy);
                        return -1;
                    }
                }

                *p = old_char;
            }

            p++;
        }

        /* create the final directory */
        if (mkdir(dir_path) != 0) {
            free(path_copy);
            return -1;
        }

        free(path_copy);
    }

    return 0;
}

/* Generate safe filename from URL or base64 data. */
static char* generate_safe_filename(const char* src, int image_counter) {
    char* filename = malloc(256);
    if (!filename) return NULL;

    if (is_base64_image(src)) {
        /* for base64 images, use counter and determine extension from MIME type */
        char* mime_type = extract_mime_type(src);
        const char* extension = get_extension_from_mime_type(mime_type);

        snprintf(filename, 256, "image_%d%s", image_counter, extension);
        if (mime_type) free(mime_type);
    }
    else {
        /* for URL images, extract filename from URL */
        const char* last_slash = strrchr(src, '/');
        const char* filename_start = last_slash ? last_slash + 1 : src;

        /* no filename in URL, use counter */
        if (filename_start[0] == '\0') snprintf(filename, 256, "image_%d.jpg", image_counter);
        else {
            /* remove query parameters and hash fragments */
            char* clean_name = malloc(strlen(filename_start) + 1);

            if (!clean_name) {
                free(filename);
                return NULL;
            }

            char* dest = clean_name;

            for (const char* p = filename_start; *p && *p != '?' && *p != '#'; p++) {
                if (isalnum(*p) || *p == '.' || *p == '-' || *p == '_')
                    *dest++ = *p;
                else
                    *dest++ = '_';
            }

            *dest = '\0';

            /* if cleaned name is empty, use counter */
            if (clean_name[0] == '\0')
                snprintf(filename, 256, "image_%d.jpg", image_counter);
            else {
                /* check if it has an extension */
                const char* dot = strrchr(clean_name, '.');

                /* no valid extension, add .jpg */
                if (!dot || dot == clean_name || (dot - clean_name) < 2)
                    snprintf(filename, 256, "%s.jpg", clean_name);
                else {
                    strncpy(filename, clean_name, 255);
                    filename[255] = '\0';
                }
            }

            free(clean_name);
        }
    }

    return filename;
}

/* Generate the hash corresponding to the filename. */
static void generate_md5_hash(const char* input, char* output) {
    /* simple hash function for generating unique filenames */
    unsigned long hash = 5381;
    int c;

    while ((c = *input++)) hash = ((hash << 5) + hash) + c;
    snprintf(output, 33, "%08lx", hash);
}

/* Generate unique filename to avoid conflicts. */
static char* generate_unique_filename(const char* base_filename, const char* src, int image_counter) {
    char* filename = generate_safe_filename(src, image_counter);
    if (!filename) return NULL;

    /* check if the file already exists */
    char test_path[1024];

#ifdef _WIN32
    snprintf(test_path, sizeof(test_path), ".\\images\\%s", filename);
#else
    snprintf(test_path, sizeof(test_path), "./images/%s", filename);
#endif

    struct stat st;

    /* file does not exist, thus will use the generated name */
    if (stat(test_path, &st) != 0) return filename;

    /* file exists, generate unique name with hash */
    free(filename);

    char hash[33];
    generate_md5_hash(src, hash);

    char* unique_name = malloc(256);
    if (!unique_name) return NULL;

    if (is_base64_image(src)) {
        char* mime_type = extract_mime_type(src);
        const char* extension = get_extension_from_mime_type(mime_type);

        snprintf(unique_name, 256, "image_%d_%s%s", image_counter, hash, extension);
        if (mime_type) free(mime_type);
    }
    else {
        const char* last_slash = strrchr(src, '/');
        const char* filename_start = last_slash ? last_slash + 1 : src;

        char base[200];
        const char* dot = strrchr(filename_start, '.');

        if (dot && dot > filename_start) {
            size_t base_len = dot - filename_start;
            if (base_len > 100) base_len = 100;

            strncpy(base, filename_start, base_len);
            base[base_len] = '\0';
            snprintf(unique_name, 256, "%s_%s%s", base, hash, dot);
        }
        else
            snprintf(unique_name, 256, "image_%d_%s.jpg", image_counter, hash);
    }

    return unique_name;
}

/* Main function to download image from src both URLs and base64 strings. */
char* download_image_src(const char* src, const char* output_dir, int image_counter) {
    if (!src || !output_dir) return NULL;

    /* create output directory if it does not exist */
    if (create_directory_if_not_exists(output_dir) != 0)
        return NULL;

    /* generate unique filename */
    char* safe_filename = generate_unique_filename(output_dir, src, image_counter);
    if (!safe_filename) return NULL;

    /* build full path */
    char* full_path = malloc(strlen(output_dir) + strlen(safe_filename) + 2);

    if (!full_path) {
        free(safe_filename);
        return NULL;
    }

    snprintf(full_path, strlen(output_dir) + strlen(safe_filename) + 2, "%s/%s", output_dir, safe_filename);
    int success = 0;

    /* handle base64 encoded image */
    if (is_base64_image(src)) 
        success = save_base64_image(src, full_path);
    /* handle normal URL */
    else success = download_image_url(src, full_path);

    free(safe_filename);

    if (success)
        return full_path;
    else {
        free(full_path);
        return NULL;
    }
}

/* Initialize libcurl stuff. */
int image_utils_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return 0;
}

/* Cleanup libcurl stuff. */
void image_utils_cleanup(void) {
    curl_global_cleanup();
}