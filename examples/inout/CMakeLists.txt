project(STATEFS_QT_EXAMPLE)
cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig REQUIRED)
pkg_check_modules(STATEFS statefs-qt5 REQUIRED)
find_package(Qt5Core REQUIRED)

IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(LIB_SUFFIX "")
ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(LIB_SUFFIX 64)
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)

set(DST_LIB lib${LIB_SUFFIX})

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -Werror -g -O0 -fPIE -std=c++0x")

include_directories(
  ${Qt5Core_INCLUDE_DIRS}
  ${STATEFS_INCLUDE_DIRS}
  ${CKIT_INCLUDE_DIRS}
)

add_definitions(
  ${Qt5Core_DEFINITIONS}
)

add_definitions(-DQT_SHARED)
set(CMAKE_AUTOMOC TRUE)

add_executable(statefs-inout-example-writer inout-writer.cpp)
target_link_libraries(statefs-inout-example-writer
  ${Qt5Core_LIBRARIES}
  ${STATEFS_LIBRARIES}
)

pkg_check_modules(CKIT contextkit-statefs REQUIRED)

add_executable(statefs-inout-example-reader inout-reader.cpp)
target_link_libraries(statefs-inout-example-reader
  ${Qt5Core_LIBRARIES}
  ${CKIT_LIBRARIES}
)

install(
  TARGETS statefs-inout-example-reader statefs-inout-example-writer
  DESTINATION ${DST_LIB}/statefs/examples)

install(
  FILES statefs-inout-example.conf
  DESTINATION share/statefs/examples)