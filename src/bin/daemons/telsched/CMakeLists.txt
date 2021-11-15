cmake_minimum_required(VERSION 3.1)
project(telsched VERSION 0.1)

set(
  SRC_FILES
  camcal.c
  cat.c
  chksch.c
  delsched.c
  edit.c
  obs.c
  obsdsp.c
  photstd.c
  pointing.c
  print.c
  #readsls.c # Removed from source
  scangraphic.c
  sortscans.c
  telsched.c
  telsched.h
  )

add_executable(telsched ${SRC_FILES})

target_link_libraries(telsched misc astro fits)
target_link_libraries(telsched ${MATH_LIBRARY})

install(TARGETS telsched DESTINATION bin)
