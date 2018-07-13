cmake_minimum_required(VERSION 3.9)
project(oscar-cmd)
find_package(LIBRT)
find_package(Cairo REQUIRED)
find_package(OpenMP)

set(OSCAR_CMD_SOURCES_CPP
	oscarmain.cpp
	Workfunctions.cpp
	Config.cpp
	Benchmark.cpp
	LiveCompleter.cpp
	GeoHierarchyPrinter.cpp
	ConsistencyCheckers.cpp
	CairoRenderer.cpp
	DecelledACOT.cpp
)

SET(MY_LINK_LIBS
	liboscar
	${CAIRO_LIBRARIES}
)

add_executable(${PROJECT_NAME} ${OSCAR_CMD_SOURCES_CPP})
target_link_libraries(${PROJECT_NAME} ${MY_LINK_LIBS})
