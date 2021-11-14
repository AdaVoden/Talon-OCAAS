cmake_minimum_required(VERSION 3.1)
project(csi VERSION 0.1)

include_directories(libmisc_SOURCE_DIR)

add_executable(csi csi.c)

install(TARGETS csi DESTINATION bin)
