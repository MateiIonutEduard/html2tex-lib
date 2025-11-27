cmake_minimum_required(VERSION 3.16)
project(html2tex
    VERSION 1.0.0
    LANGUAGES C CXX
    DESCRIPTION "HTML to LaTeX Converter - C and C++ Static Libraries"
)

# Set C/C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 03)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    set(HTML2TEX_OS_WINDOWS ON)
    set(LIBRARY_PREFIX "")
    set(LIBRARY_SUFFIX ".lib")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set(HTML2TEX_OS_UNIX ON)
    set(LIBRARY_PREFIX "lib")
    set(LIBRARY_SUFFIX ".a")
endif()

# Architecture detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_NAME "x64")
    set(ARCH_BITS 64)
else()
    set(ARCH_NAME "x86")
    set(ARCH_BITS 32)
endif()

# Compiler detection
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(C_COMPILER "GCC")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(C_COMPILER "Clang")
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(C_COMPILER "MSVC")
else()
    set(C_COMPILER "Unknown")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CXX_COMPILER "GCC")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CXX_COMPILER "Clang")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CXX_COMPILER "MSVC")
else()
    set(CXX_COMPILER "Unknown")
endif()

# Print configuration
message(STATUS "Building html2tex ${PROJECT_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${ARCH_NAME} (${ARCH_BITS}-bit)")
message(STATUS "C Compiler: ${C_COMPILER}")
message(STATUS "C++ Compiler: ${CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Handle multi-config generators (Visual Studio, Xcode)
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configurations" FORCE)
endif()

# Set output directories for different configurations and architectures
set(OUTPUT_BASE_DIR "${CMAKE_BINARY_DIR}/bin")
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config generators (VS, Xcode)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BASE_DIR}/Debug/${ARCH_NAME}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BASE_DIR}/Release/${ARCH_NAME}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BASE_DIR}/Debug/${ARCH_NAME}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BASE_DIR}/Release/${ARCH_NAME}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_BASE_DIR}/Debug/${ARCH_NAME}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_BASE_DIR}/Release/${ARCH_NAME}")
else()
    # Single-config generators (Makefiles, Ninja)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${CMAKE_BUILD_TYPE}/${ARCH_NAME}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${CMAKE_BUILD_TYPE}/${ARCH_NAME}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${CMAKE_BUILD_TYPE}/${ARCH_NAME}")
endif()

# Create output directories
file(MAKE_DIRECTORY "${OUTPUT_BASE_DIR}/Debug/${ARCH_NAME}")
file(MAKE_DIRECTORY "${OUTPUT_BASE_DIR}/Release/${ARCH_NAME}")

# Compiler flags for different configurations
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Common flags for GCC and Clang
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    
    # Platform-specific flags
    if(NOT WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif()
    
    # Configuration-specific flags
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
elseif(MSVC)
    # MSVC flags
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    
    # Configuration-specific flags
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od /MTd")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /MT")
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /MT")
    
    # Use static runtime by default
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Find required packages
find_package(PkgConfig QUIET)

# Check for libcurl (for image downloading)
if(HTML2TEX_OS_WINDOWS)
    # Windows: we'll use the bundled version or expect vcpkg/conan
    message(STATUS "libcurl: Using bundled or package manager version")
else()
    # Unix: try to find system libcurl
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(CURL QUIET libcurl)
    endif()
    if(NOT CURL_FOUND)
        find_path(CURL_INCLUDE_DIR curl/curl.h)
        find_library(CURL_LIBRARY NAMES curl)
        if(CURL_INCLUDE_DIR AND CURL_LIBRARY)
            set(CURL_FOUND TRUE)
            set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
            set(CURL_LIBRARIES ${CURL_LIBRARY})
        endif()
    endif()
    
    if(CURL_FOUND)
        message(STATUS "Found libcurl: ${CURL_LIBRARIES}")
    else()
        message(WARNING "libcurl not found - image downloading will be disabled")
        add_compile_definitions(HTML2TEX_NO_CURL=1)
    endif()
endif()

# Create C static library
add_library(html2tex_c STATIC
    source/html_parser.c
    source/html2tex.c
    source/tex_gen.c
    source/html_minify.c
    source/html_prettify.c
    source/html2tex_css.c
    source/tex_image_utils.c
)

# Set C library properties
set_target_properties(html2tex_c PROPERTIES
    OUTPUT_NAME "html2tex_c"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Include directories for C library
target_include_directories(html2tex_c
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Link libraries for C library
if(CURL_FOUND)
    target_include_directories(html2tex_c PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(html2tex_c PRIVATE ${CURL_LIBRARIES})
endif()

# Create C++ wrapper static library
add_library(html2tex_cpp STATIC
    source/html_parser.cpp
    source/html_converter.cpp
)

# Set C++ library properties
set_target_properties(html2tex_cpp PROPERTIES
    OUTPUT_NAME "html2tex_cpp"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_STANDARD 03
    CXX_STANDARD_REQUIRED ON
)

# Include directories for C++ library
target_include_directories(html2tex_cpp
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Link C++ library to C library
target_link_libraries(html2tex_cpp PUBLIC html2tex_c)

# Create aliases for easier linking
add_library(html2tex::c ALIAS html2tex_c)
add_library(html2tex::cpp ALIAS html2tex_cpp)

# Set installation paths
set(INCLUDE_INSTALL_DIR include)
set(SOURCE_INSTALL_DIR source)
set(CMAKE_INSTALL_LIBDIR lib)

# Installation for C library
install(TARGETS html2tex_c
    EXPORT html2texTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
)

# Installation for C++ library
install(TARGETS html2tex_cpp
    EXPORT html2texTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
)

# Install headers
install(FILES
    include/html2tex.h
    include/htmltex.h
    DESTINATION ${INCLUDE_INSTALL_DIR}
)

# Install C++ wrapper source for external applications
install(FILES
    source/html_parser.cpp
    sources/html_converter.cpp
    DESTINATION ${SOURCE_INSTALL_DIR}
)

# Export targets
install(EXPORT html2texTargets
    FILE html2texTargets.cmake
    NAMESPACE html2tex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/html2tex
)

# Create package configuration
include(CMakePackageConfigHelpers)

# Create the config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/html2texConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/html2texConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/html2tex
    PATH_VARS 
        INCLUDE_INSTALL_DIR
        SOURCE_INSTALL_DIR
)

# Install the config file
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/html2texConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/html2tex
)

# Export targets for build tree (for development)
export(EXPORT html2texTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/html2texTargets.cmake
    NAMESPACE html2tex::
)

# Create a version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/html2texConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the version file
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/html2texConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/html2tex
)

# Include CPack for package generation
include(CPack)

# Add custom targets for different architectures (if needed)
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Print final output information
message(STATUS "")
message(STATUS "Build configuration complete:")
message(STATUS "  C static library: ${LIBRARY_PREFIX}html2tex_c${LIBRARY_SUFFIX}")
message(STATUS "  C++ static library: ${LIBRARY_PREFIX}html2tex_cpp${LIBRARY_SUFFIX}")
message(STATUS "  Architecture: ${ARCH_NAME}")
message(STATUS "  Output directory: ${OUTPUT_BASE_DIR}/<Config>/<Arch>")
message(STATUS "  C headers: ${INCLUDE_INSTALL_DIR}/html2tex.h")
message(STATUS "  C++ headers: ${INCLUDE_INSTALL_DIR}/htmltex.h")
message(STATUS "")
message(STATUS "Installation paths:")
message(STATUS "  Headers: ${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_DIR}")
message(STATUS "  Libraries: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
message(STATUS "  Sources: ${CMAKE_INSTALL_PREFIX}/${SOURCE_INSTALL_DIR}")
message(STATUS "  CMake config: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/html2tex")
message(STATUS "")
message(STATUS "For external applications:")
message(STATUS "  C: Link against html2tex::c and include html2tex.h")
message(STATUS "  C++: Link against html2tex::cpp and include htmltex.h")
message(STATUS "")
message(STATUS "Build commands:")
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "  cmake --build . --config Release")
    message(STATUS "  cmake --build . --config Debug")
else()
    message(STATUS "  cmake --build .")
endif()
message(STATUS "  cmake --install . --prefix /path/to/install")
