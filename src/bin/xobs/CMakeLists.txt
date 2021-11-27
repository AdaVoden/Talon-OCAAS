cmake_minimum_required(VERSION 3.1)
project(xobs VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
  paddle.c
  batch.c
  calaxes.c
  config.c
  control.c
  dome.c
  fifos.c
  gui.c
  query.c
  scope.c
  skymap.c
  sound.c
  telrun.c
  tips.c
  update.c
  xephem.c
  xobs.c
  )

add_executable(xobs ${SRC_FILES})

target_link_libraries(xobs misc Xmisc astro fits wcs fs)
target_link_libraries(xobs ${MATH_LIBRARY})
target_link_libraries(xobs ${GUI_LIBS})

install(TARGETS xobs DESTINATION bin)
