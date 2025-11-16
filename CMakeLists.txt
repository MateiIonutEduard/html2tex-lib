cmake_minimum_required(VERSION 3.16)
project(html2tex
    VERSION 1.0.0
    LANGUAGES C
    DESCRIPTION "HTML to LaTeX Converter - C Static Library"
)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    set(HTML2TEX_OS_WINDOWS ON)
    set(LIBRARY_PREFIX "")
    set(LIBRARY_SUFFIX ".lib")
else()
    set(HTML2TEX_OS_UNIX ON)
    set(LIBRARY_PREFIX "lib")
    set(LIBRARY_SUFFIX ".a")
endif()

# Architecture detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_NAME "x64")
else()
    set(ARCH_NAME "x86")
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

# Print configuration
message(STATUS "Building html2tex ${PROJECT_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Architecture: ${ARCH_NAME}")
message(STATUS "C Compiler: ${C_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Create output directory
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/Release")
file(MAKE_DIRECTORY ${OUTPUT_DIR})

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Windows-specific settings
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    
    # Use static runtime by default
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
    if(NOT WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    endif()
elseif(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /O2")
endif()

# Create C static library
add_library(html2tex STATIC
    source/html_parser.c
    source/html2tex.c
    source/tex_gen.c
)

# Set library properties
set_target_properties(html2tex PROPERTIES
    OUTPUT_NAME "html2tex"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# Include directories
target_include_directories(html2tex
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Create an alias for easier linking
add_library(html2tex::html2tex ALIAS html2tex)

# Set installation paths
set(INCLUDE_INSTALL_DIR include)
set(SOURCE_INSTALL_DIR source)
set(CMAKE_INSTALL_LIBDIR lib)

# Installation
install(TARGETS html2tex
    EXPORT html2texTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
)

# Install headers
install(FILES
    include/html2tex.h
    include/HtmlToLatexConverter.h
    DESTINATION ${INCLUDE_INSTALL_DIR}
)

# Install C++ wrapper source for external applications
install(FILES
    source/HtmlToLatexConverter.cpp
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

# Export targets for build tree (optional, for development)
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

# Print final output information
message(STATUS "")
message(STATUS "Build configuration complete:")
message(STATUS "  C static library: ${OUTPUT_DIR}/${LIBRARY_PREFIX}html2tex${LIBRARY_SUFFIX}")
message(STATUS "  C headers: ${INCLUDE_INSTALL_DIR}/html2tex.h")
message(STATUS "  C++ wrapper: ${INCLUDE_INSTALL_DIR}/HtmlToLatexConverter.h + ${SOURCE_INSTALL_DIR}/HtmlToLatexConverter.cpp")
message(STATUS "")
message(STATUS "Installation paths:")
message(STATUS "  Headers: ${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_DIR}")
message(STATUS "  Library: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
message(STATUS "  Sources: ${CMAKE_INSTALL_PREFIX}/${SOURCE_INSTALL_DIR}")
message(STATUS "  CMake config: ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/html2tex")
message(STATUS "")
message(STATUS "For external applications:")
message(STATUS "  1. Link against: ${LIBRARY_PREFIX}html2tex${LIBRARY_SUFFIX}")
message(STATUS "  2. Include: html2tex.h (C) or HtmlToLatexConverter.h (C++)")
message(STATUS "  3. Add HtmlToLatexConverter.cpp to your C++ project")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  cmake --build . --config Release")
message(STATUS "  cmake --install . --prefix /path/to/install")