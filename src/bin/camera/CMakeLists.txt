cmake_minimum_required(VERSION 3.1)
project(camera VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
  arith.c
  basic.c
  blink.c
  camera.c
  corr.c
  del.c
  epoch.c
  fits.c
  glass.c
	drift.c
  gsclim.c
  hist.c
  ip.c
  luts.c
  mag.c
  main.c
  markers.c
  measure.c
	open.c
  patchlevel.c
  photom.c
  photomabs.c
  print.c
  ps.c
  query.c
  save.c
	stars.c
  wcs.c
  winhist.c
  ximage.c)

add_executable(camera ${SRC_FILES})

target_link_libraries(camera misc astro Xmisc fits wcs fs)
target_link_libraries(camera ${MATH_LIBRARY})
target_link_libraries(camera ${GUI_LIBS})

install(TARGETS camera DESTINATION bin)
