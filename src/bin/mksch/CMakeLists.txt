cmake_minimum_required(VERSION 3.1)
project(mksch VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
  gui.c
  mksch.c
  msg.c
  sched.c
  tips.c)

add_executable(mksch ${SRC_FILES})

target_link_libraries(mksch misc Xmisc astro)
target_link_libraries(mksch ${MATH_LIBRARY})
target_link_libraries(mksch ${GUI_LIBS})

install(TARGETS mksch DESTINATION bin)
