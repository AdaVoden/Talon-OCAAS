cmake_minimum_required(VERSION 3.1)
project(libmisc VERSION 0.1)

set(
  SRC_FILES
	catalogs.c
  cliserv.c
	configfile.c
	crackini.c
	csimc.c
	focustemp.c
	funcmax.c
	gaussfit.c
	lstsqr.c
	misc.c
	newton.c
	rot.c
	running.c
	strops.c
  strops.h
	telaxes.c
	telenv.c
	tts.c
  )

include_directories(${PROJ_LIBS})

add_library(misc SHARED ${SRC_FILES})
install(TARGETS misc DESTINATION lib)
