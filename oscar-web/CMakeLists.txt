cmake_minimum_required(VERSION 2.8)
project(oscar-web)

find_package(CPPCMS)
find_package(LIBRT)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} ${CPPCMS_INCLUDE_DIR})

set(OSCAR_WEB_SOURCES_CPP
oscarweb.cpp
MainHandler.cpp
IndexDB.cpp
ItemDB.cpp
GeoHierarchySubSetSerializer.cpp
CQRCompleter.cpp
CellQueryResultsSerializer.cpp
BinaryWriter.cpp
)

IF(CPPCMS_FOUND AND NOT CMAKE_CROSSCOMPILING)

SET(OSCAR_WEB_LINK_LIBS
${liboscar_BINARY_DIR}/libliboscar.so
${sserialize_BINARY_DIR}/libsserialize.a
${LIBRT_LIBRARIES}
${CPPCMS_LIBRARY}
${BOOSTER_LIBRARY}
)

add_executable(${PROJECT_NAME} ${OSCAR_WEB_SOURCES_CPP})
add_dependencies(${PROJECT_NAME} liboscar sserialize)
target_link_libraries(${PROJECT_NAME} ${OSCAR_WEB_LINK_LIBS})

ENDIF(CPPCMS_FOUND AND NOT CMAKE_CROSSCOMPILING)
