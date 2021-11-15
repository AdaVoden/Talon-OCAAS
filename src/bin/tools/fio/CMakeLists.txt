cmake_minimum_required(VERSION 3.1)
project(fio VERSION 0.1)

include_directories(${PROJ_LIBS})

add_executable(fio fio.c)

target_link_libraries(fio misc)
target_link_libraries(fio ${MATH_LIBRARY})
target_link_libraries(fio astro)

install(TARGETS fio DESTINATION bin)
