cmake_minimum_required(VERSION 3.1)
project(rund VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(rund rund.c)

target_link_libraries(rund misc astro fits)
target_link_libraries(rund ${MATH_LIBRARY})

install(TARGETS rund DESTINATION bin)
