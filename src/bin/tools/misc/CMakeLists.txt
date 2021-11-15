cmake_minimum_required(VERSION 3.1)
project(misc-bin VERSION 0.1)

set(
  SRC_FILES
  degrad.c
  hrrad.c
  jd.c
  lookup.c
  lst.c
  nisttime.c
  raddeg.c
  radhr.c
  )
# Name conflict with libmisc


include_directories(${PROJ_LIBS})

add_executable(misc-bin ${SRC_FILES})

target_link_libraries(misc-bin misc)
target_link_libraries(misc-bin ${MATH_LIBRARY})
target_link_libraries(misc-bin astro)

install(TARGETS misc-bin DESTINATION bin)

set_target_properties(misc-bin PROPERTIES OUTPUT_NAME misc)
