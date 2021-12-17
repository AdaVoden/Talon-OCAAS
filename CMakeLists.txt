cmake_minimum_required(VERSION 3.1)
project(OCAAS VERSION 0.1)
enable_testing()
include(CTest)

### Compiler settings
add_compile_options("-fcommon" "-m64")

### Library Settings

set(SRC_LIB_DIR "${PROJECT_SOURCE_DIR}/src/libs")

list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libmisc")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libXmisc")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libastro")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libfits")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libfs")
list(APPEND PROJ_LIBS "${SRC_LIB_DIR}/libwcs")

list(APPEND GUI_LIBS Xm Xpm Xt Xext Xmu X11)

### Dependencies settings
find_library(MATH_LIBRARY m    REQUIRED)
find_library(XM_LIBRARY   Xm   REQUIRED)
find_library(XPM_LIBRARY  Xpm  REQUIRED)
find_library(XT_LIBRARY   Xt   REQUIRED)
find_library(XEXT_LIBRARY Xext REQUIRED)
find_library(XMU_LIBRARY  Xmu  REQUIRED)
find_library(X11_LIBRARY  X11  REQUIRED)

### Subdirectories

add_subdirectory(src)

### Testing
# Rudimentary testing, just making sure that no binary segfaults on us
# Don't know how to make the tests pass yet, but it doesn't matter
# So long as they output

# Bin
add_test(NAME "CAMERA_RUNS" COMMAND "camera" "-h")
add_test(NAME "CSIMC_RUNS" COMMAND "csimc" "-h")
add_test(NAME "MKSCH_RUNS" COMMAND "mksch" "-h")
add_test(NAME "SHM_RUNS" COMMAND "shm"  "-h")
add_test(NAME "TELRUN_RUNS" COMMAND "telrun" "-h")
add_test(NAME "XOBS_RUNS" COMMAND "xobs" "-h")
# Tools
add_test(NAME "DYNAMICS_RUNS" COMMAND "dynamics" "-h")
add_test(NAME "FIO_RUNS" COMMAND "fio")
add_test(NAME "MNTMODEL_RUNS" COMMAND "mntmodel" "-h")
add_test(NAME "XDALICLOCK_RUNS" COMMAND "xdaliclock" "-h")
# Daemons
add_test(NAME "CAMERAD_RUNS" COMMAND "camerad" "-h")
add_test(NAME "CSIMCD_RUNS" COMMAND "csimcd" "-h")
add_test(NAME "GPSD_RUNS" COMMAND "gpsd" "-h")
add_test(NAME "RUND_RUNS" COMMAND "rund" "-h")
add_test(NAME "SHMD_RUNS" COMMAND "shm")
add_test(NAME "TELESCOPED_RUNS" COMMAND "telescoped" "-h")
add_test(NAME "TELESCOPEDCSI_RUNS" COMMAND "telescoped.csi" "-h")
add_test(NAME "TELSCHED_RUNS" COMMAND "telsched" "-h")
add_test(NAME "WXD_RUNS" COMMAND "wxd" "-h")

### Installation
set(CMAKE_INSTALL_PREFIX "/usr/local/telescope")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#755 standard
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
  OWNER_READ
  OWNER_WRITE
  OWNER_EXECUTE
  GROUP_READ
  GROUP_EXECUTE
  WORLD_READ
  WORLD_EXECUTE
  )

### Packaging
set(CPACK_PACKAGE_NAME "talon-ocaas")
set(CPACK_PACKAGE_DESCRIPTION "GUI-Enabled package of legacy Talon observatory control software")
set(CPACK_PACKAGE_CONTACT "harlan.shaw@ucalgary.ca")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local/telescope")

### Debian
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(
  CPACK_DEBIAN_PACKAGE_DEPENDS
  "libmotif-common, libxpm4, libxext6, libxmu6, libx11-6, libxt6, xfonts-75dpi")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Abandoned for now, should work without much issue
# ### RHEL

# set(
#   CPACK_RPM_PACKAGE_REQUIRES
#   "openmotif, libXpm, libXext, libXmu, libX11, libXt")

# # must be after CPACK variables
include(CPack)

### Services

install(FILES talon.conf
  DESTINATION "/etc/ld.so.conf.d")
install(FILES weather.service
  DESTINATION "/etc/systemd/system")
install(FILES telescope.service
  DESTINATION "/etc/systemd/system")
install(FILES csimc.service
  DESTINATION "/etc/systemd/system")
install(FILES telescope.sh
  DESTINATION "/etc/profile.d")
