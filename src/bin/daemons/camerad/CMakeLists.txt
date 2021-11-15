cmake_minimum_required(VERSION 3.1)
project(camerad VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(camerad camerad.c)

target_link_libraries(camerad misc astro fits )
target_link_libraries(camerad ${MATH_LIBRARY})

install(TARGETS camerad DESTINATION bin)
