cmake_minimum_required(VERSION 3.1)
project(xdaliclock VERSION 0.1)

set(
  SRC_FILES
  colors.c
  digital.c
  hsv.c
  resources.c
  usleep.c
  visual.c
  xdaliclock.c
  defaults.h
  xdaliclock.h
  version.h
  )

add_executable(xdaliclock ${SRC_FILES})

#target_link_libraries(xdaliclock Xmisc)
target_link_libraries(xdaliclock Xm Xpm Xt Xext Xmu X11)
install(TARGETS xdaliclock DESTINATION bin)
