if(${USEQT} STREQUAL "4")

configure_file(contextkit-statefs-qt4.pc.in contextkit-statefs-qt4.pc @ONLY)
configure_file(register.c.in register.c @ONLY)

add_executable(statefs-contextkit-register register.c)

elseif(${USEQT} STREQUAL "5")

configure_file(contextkit-statefs.pc.in contextkit-statefs.pc @ONLY)

endif(${USEQT} STREQUAL "4")

#-Wno-psabi is to remove next g++ warning/note:
#the mangling of 'va_list' has changed in GCC 4.4
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wno-psabi -fPIC -std=c++0x")

if(${USEQT} STREQUAL "4")

set(SRC bridge.cpp util.cpp provider.cpp)
set(HDRS bridge.hpp)
qt4_wrap_cpp(BRIDGE_MOC_SRC ${HDRS})

add_library(provider-contextkit
  SHARED
  ${SRC}
  ${BRIDGE_MOC_SRC}
)

# dirty hack: absolute path, moc is $$$
qt4_wrap_cpp(LIB_MOC_SRC /usr/include/contextsubscriber/contextproperty.h property.hpp)

target_link_libraries(provider-contextkit
  ${QT_QTCORE_LIBRARY}
  ${QT_QTXML_LIBRARY}
  ${CONTEXTSUBSCRIBER_LIBRARIES}
  ${CONTEXTPROVIDER_LIBRARIES}
)

install(TARGETS provider-contextkit DESTINATION ${DST_LIB}/statefs)

add_library(contextkit-statefs-qt4
  SHARED property.cpp util.cpp
  ${LIB_MOC_SRC}
)

target_link_libraries(contextkit-statefs-qt4
  ${QT_QTCORE_LIBRARY}
)

install(TARGETS contextkit-statefs-qt4 DESTINATION ${DST_LIB})
install(FILES contextkit-statefs-qt4.pc DESTINATION ${DST_LIB}/pkgconfig)
install(TARGETS statefs-contextkit-register
  DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE
  SETUID
  )

elseif(${USEQT} STREQUAL "5")

qt5_wrap_cpp(LIB_MOC_SRC ${CMAKE_SOURCE_DIR}/include/contextsubscriber/contextproperty.h)
# to avoid clashing with qt4 version when building in the same dir
qt5_generate_moc(property.hpp moc_property_qt5.cpp)

add_library(contextkit-statefs
  SHARED property.cpp util.cpp moc_property_qt5.cpp
  ${LIB_MOC_SRC}
)

target_link_libraries(contextkit-statefs
  ${Qt5Core_LIBRARIES}
)

install(TARGETS contextkit-statefs DESTINATION ${DST_LIB})
install(FILES contextkit-statefs.pc DESTINATION ${DST_LIB}/pkgconfig)

endif(${USEQT} STREQUAL "4")

#add_executable(testprop testprop.cpp prov.cpp)

#add_executable(test2 testimpl.cpp provider_impl.cpp)
