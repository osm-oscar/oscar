cmake_minimum_required(VERSION 2.8)
project(oscar-create)

find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ICU 4.9 REQUIRED)
find_package(LIBRT REQUIRED)
find_package(OpenMP REQUIRED)

include_directories(
	${LIBOSCAR_INCLUDE_DIR}
	${OSMPBF_INCLUDE_DIRS}
	${ICU_INCLUDE_DIRS}
	${CGAL_INCLUDE_DIRS}
	${CGAL_3RD_PARTY_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/vendor/jsoncpp/include
)

set(LIB_OSCAR_CREATE_SOURCES_CPP
	AreaExtractor.cpp
	CellCreator.cpp
	CellTextCompleter.cpp
	OsmKeyValueObjectStore.cpp
	ScoreCreator.cpp
	TagStore.cpp
	Config.cpp
	TextSearchTraits.cpp
	helpers.cpp
	${CMAKE_SOURCE_DIR}/vendor/jsoncpp/src/jsoncpp.cpp
)

set(OSCAR_CREATE_SOURCES_CPP
oscarcreate.cpp
readwritefuncs.cpp
)

set(OSCAR_CREATE_CELLS_SOURCES_CPP
	createcells.cpp
)

add_library(lib${PROJECT_NAME} STATIC ${LIB_OSCAR_CREATE_SOURCES_CPP})
add_executable(${PROJECT_NAME} ${OSCAR_CREATE_SOURCES_CPP})
add_executable(${PROJECT_NAME}-cells ${OSCAR_CREATE_CELLS_SOURCES_CPP})

SET(OSCAR_CREATE_LINK_LIBS
	lib${PROJECT_NAME}
	libosmtools
	${LIBOSCAR_LINK_LIBRARIES}
	${SSERIALIZE_LINK_LIBRARIES}
	${OSMPBF_LINK_LIBRARIES}
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
add_dependencies(${PROJECT_NAME} liboscar sserialize osmpbf libosmtools)

target_link_libraries(${PROJECT_NAME}-cells ${OSCAR_CREATE_LINK_LIBS})
add_dependencies(${PROJECT_NAME}-cells liboscar sserialize osmpbf libosmtools)

add_target_properties(lib${PROJECT_NAME} COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(lib${PROJECT_NAME} LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")

add_target_properties(${PROJECT_NAME} COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(${PROJECT_NAME} LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")

add_target_properties(${PROJECT_NAME}-cells COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(${PROJECT_NAME}-cells LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")