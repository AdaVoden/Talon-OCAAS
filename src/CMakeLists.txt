add_subdirectory(bin)
add_subdirectory(libs)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/archive DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/comm DESTINATION .)
