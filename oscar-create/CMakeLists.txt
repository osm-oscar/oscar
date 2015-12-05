cmake_minimum_required(VERSION 2.8)
project(oscar-create)

find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ICU 4.9 REQUIRED)
find_package(LIBRT REQUIRED)

set(CGAL_DONT_OVERRIDE_CMAKE_FLAGS TRUE)
# find_package(CGAL REQUIRED)

include_directories(
	${PROTOBUF_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${PROJECT_GLOBAL_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/vendor/osmpbf
	${CMAKE_SOURCE_DIR}/vendor/jsoncpp/include
	${ICU_INCLUDE_DIRS}
	${CGAL_INCLUDE_DIRS}
	${CGAL_3RD_PARTY_INCLUDE_DIRS}
)

set(OSCAR_CREATE_SOURCES_CPP
oscarcreate.cpp
AreaExtractor.cpp
CellCreator.cpp
CellTextCompleter.cpp
helpers.cpp
OsmKeyValueObjectStore.cpp
readwritefuncs.cpp
ScoreCreator.cpp
TagStore.cpp
Config.cpp
TextSearchTraits.cpp
${CMAKE_SOURCE_DIR}/vendor/jsoncpp/src/jsoncpp.cpp
)

add_executable(${PROJECT_NAME} ${OSCAR_CREATE_SOURCES_CPP} ${LIBOSMTOOLS_SOURCES_CPP})

SET(OSCAR_CREATE_LINK_LIBS
	${liboscar_BINARY_DIR}/libliboscar.so
	${sserialize_BINARY_DIR}/libsserialize.a
	${osmpbf_BINARY_DIR}/libosmpbf.a
	${PROTOBUF_LIBRARIES}
	${ZLIB_LIBRARIES}
	${ICU_LIBRARIES}
	${ICU_I18N_LIBRARIES}
	${LIBRT_LIBRARIES}
	"CGAL"
	"mpfr"
	"gmp"
	"boost_thread"
	"boost_system"
)

target_link_libraries(${PROJECT_NAME} ${OSCAR_CREATE_LINK_LIBS})
add_dependencies(${PROJECT_NAME} liboscar sserialize osmpbf)

add_target_properties(${PROJECT_NAME} COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(${PROJECT_NAME} LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")
