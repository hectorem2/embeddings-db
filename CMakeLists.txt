cmake_minimum_required(VERSION 3.12)
project(embeddings-db CXX)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/W4)
else ()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

find_package(CURL REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(PostgreSQL REQUIRED)
pkg_check_modules(LIBMAGIC REQUIRED libmagic)

if (NOT DEFINED LIBREOFFICE_ROOT_DIR)
    set(LIBREOFFICE_ROOT_DIR "/usr/lib/libreoffice")
endif()

if (NOT MSVC)
    add_compile_definitions(SAL_UNX)
endif()

set(LIBREOFFICE_INCLUDE_DIRS
    ${LIBREOFFICE_ROOT_DIR}/sdk/include
    ${PROJECT_SOURCE_DIR}/libreoffice-include)
set(LIBREOFFICE_LIBRARIES_DIRS
    ${LIBREOFFICE_ROOT_DIR}/program
    ${LIBREOFFICE_ROOT_DIR}/sdk/lib)
set(LIBREOFFICE_LIBRARIES
    "uno_sal" "uno_salhelpergcc3" "uno_purpenvhelpergcc3" "uno_cppu"
    "uno_cppuhelpergcc3")

add_subdirectory(src)