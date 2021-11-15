cmake_minimum_required(VERSION 3.1)
project(libfits VERSION 0.1)

set(
  SRC_FILES
  align2fits.c
  #edgeStats.c #fragment of file
  filters.c
  fits.c
  fits.h
  fitscorr.c
  fitscorr.h
  fitsip.c
  )

include_directories(${PROJ_LIBS})

add_library(fits SHARED ${SRC_FILES})

install(TARGETS fits DESTINATION lib)
