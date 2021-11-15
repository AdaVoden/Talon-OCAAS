cmake_minimum_required(VERSION 3.1)
project(wxd VERSION 0.1)

set(
  SRC_FILES
  davis.c
  dd.h
  page.c
  url.c
  wxd.c
  )

add_executable(wxd ${SRC_FILES})

target_link_libraries(wxd misc astro fits)
target_link_libraries(wxd ${MATH_LIBRARY})

install(TARGETS wxd DESTINATION bin)
