cmake_minimum_required(VERSION 3.1)
project(csimc VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
  boot.c
  csimc.c
  eintrio.c
  el.c)

add_executable(csimc ${SRC_FILES})

target_link_libraries(csimc misc astro)
target_link_libraries(csimc ${MATH_LIBRARY})

install(TARGETS csimc DESTINATION bin)
