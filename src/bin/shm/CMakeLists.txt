cmake_minimum_required(VERSION 3.1)
project(shm VERSION 0.1)

add_executable(shm shm.c)

include_directories(${PROJ_LIBS})

target_link_libraries(shm misc astro Xmisc fits wcs fs)
target_link_libraries(shm ${MATH_LIBRARY})
target_link_libraries(shm ${GUI_LIBS})

install(TARGETS shm DESTINATION bin)
