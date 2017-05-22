cmake_minimum_required(VERSION 2.8)
project(oscar-cmd)
find_package(LIBRT REQUIRED)

include_directories(
	${LIBOSCAR_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)

set(OSCAR_CMD_SOURCES_CPP
oscarmain.cpp
Workfunctions.cpp
Config.cpp
Benchmark.cpp
LiveCompleter.cpp
GeoHierarchyPrinter.cpp
ConsistencyCheckers.cpp
CairoRenderer.cpp
)

SET(OSCAR_CMD_LINK_LIBS
	${LIBOSCAR_LINK_LIBRARIES}
	"cairo"
)

add_executable(${PROJECT_NAME} ${OSCAR_CMD_SOURCES_CPP})
add_dependencies(${PROJECT_NAME} liboscar)
target_link_libraries(${PROJECT_NAME} ${OSCAR_CMD_LINK_LIBS})


add_target_properties(sserialize COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(sserialize LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")

add_target_properties(liboscar COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(liboscar LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")

add_target_properties(${PROJECT_NAME} COMPILE_FLAGS "-fopenmp -DCGAL_HAS_THREADS")
add_target_properties(${PROJECT_NAME} LINK_FLAGS "-DCGAL_HAS_THREADS -fopenmp")

