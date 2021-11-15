cmake_minimum_required(VERSION 3.1)
project(telescoped.csi VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
  axes.c
  csimc.c
  dome.c
  filter.c
  fifoio.c
  focus.c
  lights.c
  mountcor.c
  powerfail.c
  tel.c
  teled.h
  telescoped.c
  virmc.c
  virmc.h
  )

add_executable(telescoped.csi ${SRC_FILES})

target_link_libraries(telescoped.csi misc astro fits)
target_link_libraries(telescoped.csi ${MATH_LIBRARY})

install(TARGETS telescoped.csi DESTINATION bin)
