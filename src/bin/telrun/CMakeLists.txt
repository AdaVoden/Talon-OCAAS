cmake_minimum_required(VERSION 3.1)
project(telrun VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
  fifoio.c
  fileio.c
  telrun.c
  pr_bias.c
  pr_flat.c
  pr_regscan.c
  pr_thermal.c
  )

add_executable(telrun ${SRC_FILES})

target_link_libraries(telrun misc astro fits)
target_link_libraries(telrun ${MATH_LIBRARY})

install(TARGETS telrun DESTINATION bin)
