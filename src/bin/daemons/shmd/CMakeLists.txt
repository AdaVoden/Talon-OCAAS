cmake_minimum_required(VERSION 3.1)
project(shmd VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(shmd shmd.c)

target_link_libraries(shmd misc astro fits)
target_link_libraries(shmd ${MATH_LIBRARY})

install(TARGETS shmd DESTINATION bin)
