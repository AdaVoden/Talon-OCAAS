cmake_minimum_required(VERSION 3.1)
project(libXmisc VERSION 0.1)

set(
  SRC_FILES
  catbrowse.c
  xtools.c
  )

include_directories(${PROJ_LIBS})

add_library(Xmisc SHARED ${SRC_FILES})

install(TARGETS Xmisc DESTINATION lib)
