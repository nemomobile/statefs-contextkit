PROJECT(statefs-contextkit)
cmake_minimum_required(VERSION 2.8)

IF(NOT DEFINED STATEFS_QT_VERSION)
MESSAGE(FATAL_ERROR "Define STATEFS_QT_VERSION")
ENDIF(NOT DEFINED STATEFS_QT_VERSION)

option(ENABLE_MULTIARCH "Enable support for multi-arch distros (lib64)" OFF)
IF(ENABLE_MULTIARCH)
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX "")
  ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
ELSE()
  SET(LIB_SUFFIX "")
ENDIF()

set(DST_LIB lib${LIB_SUFFIX})
set(prefix ${CMAKE_INSTALL_PREFIX})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(PkgConfig REQUIRED)


if(${USEQT} STREQUAL "4")

find_package(Qt4 4.7.0 REQUIRED)
include(${QT_USE_FILE})
add_definitions(-DQT_SHARED)
include_directories(
  ${QT_INCLUDE_DIR}
)
install(DIRECTORY include/statefs DESTINATION include/qt4)

elseif(${USEQT} STREQUAL "5")

find_package(Qt5Core REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})
add_definitions(${Qt5Core_DEFINITIONS})
install(DIRECTORY include/statefs DESTINATION include/qt5)

endif(${USEQT} STREQUAL "4")

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -O2 -feliminate-unused-debug-types -std=c++0x"
  )

#-Wno-psabi is to remove next g++ warning/note:
#the mangling of 'va_list' has changed in GCC 4.4
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wno-psabi"
)

# fix for gcc 4.6 specific compiling/linking issues, no dwarf-4
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -gdwarf-3"
  )


pkg_check_modules(COR cor REQUIRED)
pkg_check_modules(STATEFS statefs REQUIRED)


INCLUDE_DIRECTORIES(
  ${COR_INCLUDE_DIRS}
  ${STATEFS_INCLUDE_DIRS}
)

link_directories(
  ${COR_LIBRARY_DIRS}
  ${STATEFS_LIBRARY_DIRS}
)

if(${USEQT} STREQUAL "4")

pkg_check_modules(CONTEXTSUBSCRIBER contextsubscriber-1.0 REQUIRED)
pkg_check_modules(CONTEXTPROVIDER contextprovider-1.0 REQUIRED)

include_directories(
  ${CONTEXTSUBSCRIBER_INCLUDE_DIRS}
  ${CONTEXTPROVIDER_INCLUDE_DIRS}
)

link_directories(
  ${CONTEXTSUBSCRIBER_LIBRARY_DIRS}
  ${CONTEXTPROVIDER_LIBRARY_DIRS}
)

else(${USEQT} STREQUAL "4")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/contextsubscriber)
install(FILES include/contextsubscriber/contextproperty.h DESTINATION include)

endif(${USEQT} STREQUAL "4")

add_custom_target(doc
  COMMAND doxygen ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile)

add_subdirectory(src)
add_subdirectory(tests)
