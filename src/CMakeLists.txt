add_subdirectory(bin)
add_subdirectory(libs)
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/archive
  DESTINATION .
  PERMISSIONS WORLD_READ WORLD_WRITE)
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/comm
  DESTINATION .
  PERMISSIONS WORLD_READ WORLD_WRITE)
