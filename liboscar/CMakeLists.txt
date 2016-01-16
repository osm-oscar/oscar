cmake_minimum_required(VERSION 2.8)
project(liboscar)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(LIBRT REQUIRED)

set(LIBOSCAR_SOURCES_CPP
StaticOsmCompleter.cpp
StaticTagStore.cpp
constants.cpp
tagcompleters.cpp
SetOpTreePrivateGeo.cpp
OsmKeyValueObjectStore.cpp
TextSearch.cpp
GeoSearch.cpp
CellOpTree.cpp
AdvancedCellOpTree.cpp
)

add_target_properties(sserialize COMPILE_FLAGS -fPIC)


SET(LIBOSCAR_LINK_LIBS
	${sserialize_BINARY_DIR}/libsserialize.a
	${LIBRT_LIBRARIES}
	"CGAL"
	"mpfr"
	"gmp"
	"boost_thread"
	"boost_system"
)

add_library(${PROJECT_NAME} SHARED ${LIBOSCAR_SOURCES_CPP})
target_link_libraries(${PROJECT_NAME} ${LIBOSCAR_LINK_LIBS}) 
add_dependencies(${PROJECT_NAME} sserialize)