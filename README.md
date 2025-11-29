# html2tex

**Lightning-fast HTML → LaTeX conversion in pure C & modern C++**<br/>
Static, cross-platform, dependency-light libraries for fast document transformation.

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![C++14](https://img.shields.io/badge/C++-14-blue.svg)](https://en.cppreference.com/w/cpp/14)
[![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://en.wikipedia.org/wiki/Cross-platform)

## 📚 Architecture Overview

The repository provides two independent static libraries:

### 🔹 html2tex_c — Core C Library

Located in `/source` and `/include/html2tex.h`.

Features:
* High-performance *HTML5* subset parsing

* Inline *CSS* 2.1 core support (colors, weight, alignment, spacing, etc.)

* **T**e**X**/**L**a**T**e**X** code generation

* Optional static `libcurl` integration (image downloading or external resources)

* No external dependencies besides optional `libcurl`

* Fully cross-platform

### 🔹 html2tex_cpp — Modern C++14 Wrapper

Located in `/source` and `/include/htmltex.h`.
Features:

* RAII interface over the C library

* Easier integration into C++ applications

* Clean OOP API (`HtmlParser`, `HtmlTeXConverter`)

* Statically links against `html2tex_c`<br/>

## 🚀 Quick Start

```bash
# Clone & build
git clone https://github.com/MateiIonutEduard/html2tex.git && cd html2tex
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel --config Release
```

Outputs are generated in:

```bash
/bin/<Debug|Release>/<x64|x86>/
```

## 💻 Usage Examples
### C API (`html2tex_c`)

```c
#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s.\n", filename);
        return NULL;
    }

    /* get file size */
    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* allocate memory */
    char* content = malloc(file_size + 1);

    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    /* read file content */
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';

    fclose(file);
    return content;
}

int main(int argc, char** argv) {
   /* check command line arguments */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <html_file_path> <output_image_directory>\n", argv[0]);
        return 1;
    }

    LaTeXConverter* converter = html2tex_create();
    html2tex_set_image_directory(converter, argv[2]);

    html2tex_set_download_images(converter, 1);
    char* html_data = GetHTML(argv[1]);
    char* latex = html2tex_convert(converter, html_data);

    if (latex) {
        printf("\"%s\"\n", latex);
        free(latex);
    }

    html2tex_destroy(converter);
    return 0;
}
```

### C++ API (`html2tex_cpp`)

```cpp
#include "htmltex.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    /* check command line arguments */
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <html_file_path> <output_image_directory>" << std::endl;
        return 1;
    }

	std::ifstream in(argv[1]);
	std::ostringstream stream;
	stream << in.rdbuf();

	std::string str = stream.str();
	in.close();

	HtmlTeXConverter util;
	util.setDirectory(argv[2]);

	std::string latex = util.convert(str);
	std::cout << latex << std::endl;
	return 0;
}
```

## 📁 Repository Layout

```css
html2tex/
├── include/
│   ├── html2tex.h       # C API
│   └── htmltex.h        # C++ API wrapper
├── source/
│   ├── html2tex.c
│   ├── html2tex_css.c
│   ├── html_parser.c
│   ├── html_minify.c
│   ├── html_prettify.c
│   ├── tex_gen.c
│   ├── tex_image_utils.c
│   ├── html_converter.cpp
│   └── html_parser.cpp
├── cmake/
│   └── html2texConfig.cmake.in
├── LICENSE
├── README.md
└── CMakeLists.txt
```

## 🔧 Integration Options

### 1. Direct Inclusion (simplest)<br/>
Copy these interfaces (C/C++):<br/>
* `include/html2tex.h`
* `include/htmltex.h`<br/>

The built libraries:
* `html2tex_c.lib/.a`
* `html2tex_cpp.lib/.a`<br/>

Example CMake:

```cmake
target_include_directories(your_target PUBLIC third_party/html2tex/include)

target_link_libraries(your_target PUBLIC
    third_party/html2tex/bin/<Debug|Release>/<x64|x86>/html2tex_c
    third_party/html2tex/bin/<Debug|Release>/<x64|x86>/html2tex_cpp
)
```

### 2. CMake Package (recommended)<br/>

After installation:<br/>

```cmake
find_package(html2tex REQUIRED)

target_link_libraries(your_target
    PUBLIC html2tex::c
    PUBLIC html2tex::cpp
)
```

Targets provided:<br/>
* `html2tex::c` → C static library
* `html2tex::cpp` → C++ wrapper<br/>

Installed structure:<br/>

```bash
include/html2tex.h
include/htmltex.h
bin/<Debug|Release>/<x64|x86>/libhtml2tex_c.a
bin/<Debug|Release>/<x64|x86>/lib/libhtml2tex_cpp.a
cmake/html2tex/html2texConfig.cmake
```

### 3. Add Subdirectory<br/>

```cmake
add_subdirectory(third_party/html2tex)

target_link_libraries(your_target
    PUBLIC html2tex::c
    PUBLIC html2tex::cpp
)
```

## 🖥️ Platform Compatibility
* Windows: MSVC 2019+, MinGW-w64
* Linux: GCC 9+, Clang 10+
* Mac OSX: Clang (Xcode 12+)<br/>

Auto-detects:<br/>
* OS and Compiler
* libcurl availability
* Architecture (x86/x64)<br/>

Produces architecture-specific output directories.<br/>


## 🛠️ Installation

### Linux / Mac OSX:<br/>

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
sudo cmake --install . --prefix /usr/local
```

### Windows (Visual Studio 2022):<br/>

```cmd
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cmake --install . --prefix "C:\Libraries\html2tex"
```

## 🎯 Why html2tex?
* 🔄 Lightweight Dependencies - Pure C/C++ core, uses libcurl for image downloads
* ⚡ High Performance - Optimized parsing and conversion
* 🎯 Cross-Platform - Consistent behavior everywhere
* 🔧 Dual Interface - C API + modern C++14 wrapper
* 📦 Static Linking - Single binary deployment<br/>

## 🛡️ Compatibility Notes

- ✅ **Rich HTML5 & CSS 2.1** - Parses a substantial subset of elements and core properties
- 🔒 **Automatic Escaping** - Intelligent LaTeX character handling  
- 🏗️ **Nested Element Support** - Robust scope management
- 🌐 **Cross-Platform Consistency** - Identical behavior everywhere
- 🔧 **Extensible Mappings** - Custom conversions via source code
- 💡 **Graceful Degradation** - Unsupported elements preserved as content<br/>

**Note:** Unsupported **HTML** elements are gracefully ignored while preserving all content.
