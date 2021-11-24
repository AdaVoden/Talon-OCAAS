add_subdirectory(bin)
add_subdirectory(libs)
add_subdirectory(archive)
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/comm
  DESTINATION .
  USE_SOURCE_PERMISSIONS)
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/comm
  DESTINATION .
  USE_SOURCE_PERMISSIONS)
