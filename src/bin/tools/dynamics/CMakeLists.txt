cmake_minimum_required(VERSION 3.1)
project(dynamics VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(dynamics dynamics.c)

target_link_libraries(dynamics ${MATH_LIBRARY})

install(TARGETS dynamics DESTINATION bin)
