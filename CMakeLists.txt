cmake_minimum_required(VERSION 3.6)
PROJECT(oscar-meta)
SET(PACKAGE_NAME ${CMAKE_PROJECT_NAME})
SET(PACKAGE_VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

set(CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/cmake
	${CMAKE_SOURCE_DIR}/liboscar/vendor/sserialize/cmake/
	${CMAKE_SOURCE_DIR}/vendor/sserialize/cmake/
)

option(OSCAR_OPTIMIZE_ULTRA "Build oscar-web with lto and aggressive optimization settings for sserialize")
if (OSCAR_OPTIMIZE_ULTRA)
	set(LIBOSCAR_NO_DATA_REFCOUNTING_ENABLED TRUE)
	set(SSERIALIZE_INLINE_IN_LTO_ENABLED TRUE)
	set(SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED TRUE)
	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# set -Wshadow-compatible-local as soon as supported
set(MY_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unknown-pragmas -Wstrict-overflow=3")
set(MY_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unknown-pragmas -Wstrict-overflow=3")# -Wshorten-64-to-32")

option(OSCAR_BUILD_WITH_SANITIZERS "Build binaries with additional instrumentation (address, undefined behaviour, thread) sanitizer" OFF)
if (OSCAR_BUILD_WITH_SANITIZERS)
	set(MY_C_FLAGS "${MY_C_FLAGS} -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
	set(MY_CXX_FLAGS "${MY_C_FLAGS} -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
endif()

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
ELSE(CMAKE_CROSSCOMPILING OR ANDROID)
	add_subdirectory(liboscar liboscar) #contains sserialize
	
	#these only need liboscar
	add_subdirectory(oscar-cmd oscar-cmd)
	add_subdirectory(oscar-gui oscar-gui)

	#oscar-create is only useful on 64 Bit systems
	IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
		add_subdirectory(vendor/osmpbf osmpbf)
		add_subdirectory(vendor/libosmtools libosmtools)
		add_subdirectory(oscar-create oscar-create)
	ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	
ENDIF(CMAKE_CROSSCOMPILING OR ANDROID)
