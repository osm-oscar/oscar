cmake_minimum_required(VERSION 3.9)
project(oscar-create)
find_package(LIBRT REQUIRED)
find_package(OpenMP)

if (LIBOSCAR_NO_DATA_REFCOUNTING_ENABLED)
	set(MY_COMPILE_DEFINITIONS
		"-DWITH_OSCAR_CREATE_NO_DATA_REFCOUNTING"
		${MY_COMPILE_DEFINITIONS}
	)
endif(LIBOSCAR_NO_DATA_REFCOUNTING_ENABLED)

set(MY_INCLUDE_DIRS
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

SET(MY_LINK_LIBS
	libosmtools
	liboscar
	sserialize
	osmpbf
	${LIBRT_LIBRARIES}
)

add_library(lib${PROJECT_NAME} STATIC ${LIB_OSCAR_CREATE_SOURCES_CPP})
target_include_directories(lib${PROJECT_NAME} PUBLIC ${MY_INCLUDE_DIRS})
target_compile_definitions(lib${PROJECT_NAME} PUBLIC ${MY_COMPILE_DEFINITIONS})
target_link_libraries(lib${PROJECT_NAME} ${MY_LINK_LIBS})

add_executable(${PROJECT_NAME} ${OSCAR_CREATE_SOURCES_CPP})
target_link_libraries(${PROJECT_NAME} lib${PROJECT_NAME})
