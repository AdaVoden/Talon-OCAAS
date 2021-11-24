add_subdirectory(bin)
add_subdirectory(libs)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/archive DESTINATION .)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/comm DESTINATION .)
