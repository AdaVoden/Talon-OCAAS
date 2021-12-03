cmake_minimum_required(VERSION 3.1)
project(libmisc VERSION 0.1)

set(
  SRC_FILES
	catalogs.c
  catalogs.h
  ccdcamera.c
  ccdcamera.h
  cliserv.c
  cliserv.h
	configfile.c
  configfile.h
	crackini.c
  crackini.h
	csimc.c
  csimc.h
  dio.h
	focustemp.c
  focustemp.h
	funcmax.c
	gaussfit.c
	lstsqr.c
  lstsqr.h
	misc.c
  misc.h
	newton.c
	rot.c
  rot.h
  photstd.c
  photstd.h
	running.c
  running.h
  scan.c
  scan.h
	strops.c
  strops.h
	telaxes.c
	telenv.c
  telenv.h
  telfits.c
  telfits.h
  telstatshm.h
	tts.c
  tts.h
  )

add_library(misc SHARED ${SRC_FILES})
target_link_libraries(misc fits astro)

include_directories(${PROJ_LIBS})

install(TARGETS misc DESTINATION lib)
