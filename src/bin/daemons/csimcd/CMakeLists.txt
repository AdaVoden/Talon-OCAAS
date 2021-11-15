cmake_minimum_required(VERSION 3.1)
project(csimcd VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(csimcd csimcd.c)

target_link_libraries(csimcd misc astro fits)
target_link_libraries(csimcd ${MATH_LIBRARY})

install(TARGETS csimcd DESTINATION bin)
