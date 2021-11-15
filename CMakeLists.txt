cmake_minimum_required(VERSION 3.1)
project(OCAAS VERSION 0.1)

### Compiler settings
add_compile_options("-fcommon" "-m64")


### Library Settings

set(SRC_LIB_DIR "${PROJECT_SOURCE_DIR}/src/libs")

list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libmisc")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libXmisc")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libastro")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libfits")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libfs")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libwcs")
find_library(MATH_LIBRARY m)

list(APPEND GUI_LIBS Xm Xpm Xt Xext Xmu X11)

### Subdirectory settings

add_subdirectory(src)
