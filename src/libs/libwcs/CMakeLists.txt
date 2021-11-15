cmake_minimum_required(VERSION 3.1)
project(libwcs VERSION 0.1)

set(
  SRC_FILES
  align2wcs.c
  checkwcsfits.c
  delwcsfits.c
  findreg.c
  findregd.c
  findregd.h
  resetwcs.c
  setwcsfits.c
  setwcsfitsd.h
  wcs.h
  worldpos.c
  xyradec.c
  )

add_library(wcs SHARED ${SRC_FILES})

include_directories(${PROJ_LIBS})

install(TARGETS wcs DESTINATION lib)
