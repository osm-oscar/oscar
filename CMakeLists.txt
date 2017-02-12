cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/cmake
	${CMAKE_SOURCE_DIR}/liboscar/vendor/sserialize/cmake/
	${CMAKE_SOURCE_DIR}/vendor/sserialize/cmake/
)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/osmpbf
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/osmpbf/osmpbf/include
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/libosmtools/include
	${CMAKE_CURRENT_SOURCE_DIR}/liboscar/include
	${CMAKE_CURRENT_SOURCE_DIR}/liboscar/vendor/sserialize/include
	${LIBDTS2_INCLUDE_DIR}
)

include(add_buildtype_lto)

PROJECT(oscar-meta)
SET(PACKAGE_NAME ${CMAKE_PROJECT_NAME})
SET(PACKAGE_VERSION 0.1)

set(MY_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Wno-unknown-pragmas -Wstrict-overflow=3")
set(MY_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -Wextra -Wno-unknown-pragmas -Wstrict-overflow=3")# -Wshorten-64-to-32")
set(DEBUG_FLAGS "${DEBUG_FLAGS} -DDEBUG_CHECK_ALL -DWITH_SSERIALIZE_CHEAP_ASSERT")# -DWITH_SSERIALIZE_EXPENSIVE_ASSERT")# -Wconversion")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MY_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MY_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${DEBUG_FLAGS}")


IF(CMAKE_CROSSCOMPILING OR ANDROID)
	IF(ANDROID)
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -llog")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -llog")
	ENDIF(ANDROID)
	add_subdirectory(liboscar liboscar)
	add_subdirectory(oscar-cmd oscar-cmd)
ELSE (CMAKE_CROSSCOMPILING OR ANDROID)
	add_subdirectory(liboscar liboscar) #contains sserialize

	#oscar-create
	add_subdirectory(vendor/osmpbf osmpbf)
	add_subdirectory(vendor/libosmtools libosmtools)
	add_subdirectory(oscar-create oscar-create)
	
	#these only need liboscar
	add_subdirectory(oscar-cmd oscar-cmd)
	add_subdirectory(oscar-gui oscar-gui)
ENDIF(CMAKE_CROSSCOMPILING OR ANDROID)