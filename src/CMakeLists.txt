add_subdirectory(bin)
add_subdirectory(libs)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/archive DESTINATION .)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/comm DESTINATION .)
