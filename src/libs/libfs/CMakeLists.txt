cmake_minimum_required(VERSION 3.1)
project(libfs VERSION 0.1)

set(
  SRC_FILES
  fieldstar.h
	gsc.c
	sadump.c
  usno.c
  )

include_directories(${PROJ_LIBS})

add_library(fs SHARED ${SRC_FILES})

install(TARGETS fs DESTINATION lib)
