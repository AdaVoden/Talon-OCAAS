cmake_minimum_required(VERSION 3.1)
project(mntmodel VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(mntmodel mntmodel.c)

target_link_libraries(mntmodel misc)
target_link_libraries(mntmodel ${MATH_LIBRARY})
target_link_libraries(mntmodel astro)

install(TARGETS mntmodel DESTINATION bin)
