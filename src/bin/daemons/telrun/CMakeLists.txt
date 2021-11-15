cmake_minimum_required(VERSION 3.1)
project(telrun VERSION 0.1)

set(
  SRC_FILES
  fifoio.c
  fileio.c
  telrun.c
  telrun.h
  )

add_executable(telrun ${SRC_FILES})

target_link_libraries(telrun misc astro fits)
target_link_libraries(telrun ${MATH_LIBRARY})

install(TARGETS telrun DESTINATION bin)
