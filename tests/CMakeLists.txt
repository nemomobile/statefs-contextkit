if(${USEQT} STREQUAL "4")

include_directories(
  ${QT_INCLUDE_DIR}
  ${CONTEXTSUBSCRIBER_INCLUDE_DIRS}
)
set(SUBSCRIBER_LIB contextkit-statefs-qt4)

elseif(${USEQT} STREQUAL "5")

set(SUBSCRIBER_LIB contextkit-statefs)

endif(${USEQT} STREQUAL "4")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(atest-statefs_contextkit_subscriber_linking
  statefs_contextkit_subscriber_linking.cpp)

if(${USEQT} STREQUAL "4")
target_link_libraries(atest-statefs_contextkit_subscriber_linking
  ${QT_QTCORE_LIBRARY}
  ${SUBSCRIBER_LIB}
)
elseif(${USEQT} STREQUAL "5")
target_link_libraries(atest-statefs_contextkit_subscriber_linking
  ${Qt5Core_LIBRARIES}
  ${SUBSCRIBER_LIB}
)
endif(${USEQT} STREQUAL "4")
