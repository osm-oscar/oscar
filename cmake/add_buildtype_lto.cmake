cmake_minimum_required(VERSION 3.0)

IF(CMAKE_BUILD_TYPE MATCHES "^lto$")
	include(setup-lto)
ENDIF()

IF ( (NOT LTO_BUILD_TYPE_ADDED) )

	SET(CMAKE_CXX_FLAGS_LTO
		"-DNDEBUG -g -O3 -march=native -flto -fno-fat-lto-objects -frounding-math"
		CACHE STRING "Flags used by the C++ compiler during lto builds."
		FORCE
	)
	SET(CMAKE_C_FLAGS_LTO
		"-DNDEBUG -g -O3 -march=native -flto -fno-fat-lto-objects -frounding-math"
		CACHE STRING "Flags used by the C compiler during lto builds."
		FORCE
	)
	SET(CMAKE_EXE_LINKER_FLAGS_LTO
		""
		CACHE STRING "Flags used for linking binaries during lto builds."
		FORCE
	)
	SET(CMAKE_SHARED_LINKER_FLAGS_LTO
		""
		CACHE STRING "Flags used by the shared libraries linker during lto builds."
		FORCE
	)

	MARK_AS_ADVANCED(
		CMAKE_CXX_FLAGS_LTO
		CMAKE_C_FLAGS_LTO
		CMAKE_EXE_LINKER_FLAGS_LTO
		CMAKE_SHARED_LINKER_FLAGS_LTO
	)

	SET(CMAKE_CONFIGURATION_TYPES  
		"${CMAKE_CONFIGURATION_TYPES} lto"
	)

	SET(LTO_BUILD_TYPE_ADDED
		"True"
		CACHE INTERNAL ""
		FORCE
	)
ENDIF()
