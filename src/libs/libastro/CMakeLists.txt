cmake_minimum_required(VERSION 3.1)
project(libastro VERSION 0.1)

set(
  SRC_FILES
	aa_hadec.c
	aberration.c
	actan.c
	airmass.c
	anomaly.c
	ap_as.c
	auxil.c
	chap95.c
	chap95_data.c
	circum.c
	comet.c
	dbfmt.c
	deep.c
	deltat.c
	earthsat.c
	eq_ecl.c
	eq_gal.c
	formats.c
	helio.c
	libration.c
	misc.c
	mjd.c
	moon.c
	mooncolong.c
	nutation.c
	obliq.c
	parallax.c
	plans.c
	precess.c
	reduce.c
	refract.c
	riset.c
	riset_cir.c
	sdp4.c
	sgp4.c
	sphcart.c
	sun.c
	thetag.c
	utc_gst.c
	vsop87.c
	vsop87_data.c
  )

add_library(astro SHARED ${SRC_FILES})

install(TARGETS astro DESTINATION lib)
