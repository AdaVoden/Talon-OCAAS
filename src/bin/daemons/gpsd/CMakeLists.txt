cmake_minimum_required(VERSION 3.1)
project(gpsd VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(gpsd gpsd.c)

target_link_libraries(gpsd misc astro fits)
target_link_libraries(gpsd ${MATH_LIBRARY})

install(TARGETS gpsd DESTINATION bin)
