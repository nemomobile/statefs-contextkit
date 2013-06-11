PROJECT(statefs-contextkit)
cmake_minimum_required(VERSION 2.6)

IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(LIB_SUFFIX "")
ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
  SET(LIB_SUFFIX 64)
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)

set(DST_LIB lib${LIB_SUFFIX})

find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -O2 -feliminate-unused-debug-types -std=c++0x"
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

if(${USEQT} STREQUAL "5")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include/contextsubscriber)
install(FILES include/contextsubscriber/contextproperty.h DESTINATION include)
endif(${USEQT} STREQUAL "5")

add_subdirectory(src)
add_subdirectory(tests)
