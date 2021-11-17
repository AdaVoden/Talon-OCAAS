cmake_minimum_required(VERSION 3.1)
project(telsched VERSION 0.1)

include_directories(${PROJ_LIBS})

set(
  SRC_FILES
	camcal.c
	cat.c
	delsched.c
	edit.c
	obs.c
	obsdsp.c
	photstd.c
	pointing.c
	print.c
	readsls.c
	scangraphic.c
	sortscans.c
	telsched.c
  )

add_executable(telsched ${SRC_FILES})

target_link_libraries(telsched misc astro fits Xmisc)
target_link_libraries(telsched ${MATH_LIBRARY})
target_link_libraries(telsched ${GUI_LIBS})

install(TARGETS telsched DESTINATION bin)
