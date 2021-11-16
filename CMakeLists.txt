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

list(APPEND GUI_LIBS Xm Xpm Xt Xext Xmu X11)

### Dependencies settings
find_library(MATH_LIBRARY m    REQUIRED)
find_library(XM_LIBRARY   Xm   REQUIRED)
find_library(XPM_LIBRARY  Xpm  REQUIRED)
find_library(XT_LIBRARY   Xt   REQUIRED)
find_library(XEXT_LIBRARY Xext REQUIRED)
find_library(XMU_LIBRARY  Xmu  REQUIRED)
find_library(X11_LIBRARY  X11  REQUIRED)

### Subdirectory

add_subdirectory(src)

### Installation
set(CMAKE_INSTALL_PREFIX "/usr/local/telescope")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
include(CPack)

### Debian

set(
  CPACK_DEBIAN_PACKAGE_DEPENDS
  libmotif
  libxpm
  libxext
  libxmu
  libdb)

set(
  CPACK_DEBIAN_CONTACT
  "harlan.shaw@ucalgary.ca")

### RHEL
