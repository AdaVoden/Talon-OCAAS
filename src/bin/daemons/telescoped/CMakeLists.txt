cmake_minimum_required(VERSION 3.1)
project(telescoped VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(telescoped telescoped.c)

target_link_libraries(telescoped misc astro fits)
target_link_libraries(telescoped ${MATH_LIBRARY})

install(TARGETS telescoped DESTINATION bin)
