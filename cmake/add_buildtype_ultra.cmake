cmake_minimum_required(VERSION 3.5)

SET(CMAKE_CXX_FLAGS_ULTRA
	"-DNDEBUG -g -O3 -march=native -flto -fno-fat-lto-objects"
	CACHE STRING "Flags used by the C++ compiler during ultra builds."
	FORCE
)

SET(CMAKE_C_FLAGS_ULTRA
	"-DNDEBUG -g -O3 -march=native -flto -fno-fat-lto-objects"
	CACHE STRING "Flags used by the C compiler during ultra builds."
	FORCE
)

SET(CMAKE_EXE_LINKER_FLAGS_ULTRA
	""
	CACHE STRING "Flags used for linking binaries during ultra builds."
	FORCE
)
SET(CMAKE_SHARED_LINKER_FLAGS_ULTRA
	""
	CACHE STRING "Flags used by the shared libraries linker during ultra builds."
	FORCE
)

MARK_AS_ADVANCED(
	CMAKE_CXX_FLAGS_ULTRA
	CMAKE_C_FLAGS_ULTRA
	CMAKE_EXE_LINKER_FLAGS_ULTRA
	CMAKE_SHARED_LINKER_FLAGS_ULTRA
)

set(CMAKE_CONFIGURATION_TYPES  
	"${CMAKE_CONFIGURATION_TYPES} ultra"
)

IF(CMAKE_BUILD_TYPE MATCHES "^ultra$")
	include(setup-lto)

	SET(LIBOSCAR_NO_DATA_REFCOUNTING_ENABLED
		True
		CACHE
		BOOL
		"Disable data refcounting in liboscar"
		FORCE
	)
	
	set(SSERIALIZE_INLINE_IN_LTO_ENABLED
		True
		CACHE
		BOOL
		"Enable manual inline statements in lto builds"
		FORCE
	)

	SET(SSERIALIZE_CONTIGUOUS_UBA_ONLY_SOFT_FAIL_ENABLED
		True
		CACHE
		BOOL
		"Disable non-contiguous UBA"
		FORCE
	)
ENDIF()
