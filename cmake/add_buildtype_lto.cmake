cmake_minimum_required(VERSION 3.0)

SET(CMAKE_CXX_FLAGS_LTO
	"-DNDEBUG -O3 -march=native -flto -ffat-lto-objects -frounding-math"
	CACHE STRING "Flags used by the C++ compiler during lto builds."
	FORCE
)
SET(CMAKE_C_FLAGS_LTO
	"-DNDEBUG -O3 -march=native -flto -ffat-lto-objects -frounding-math"
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

set(CMAKE_CONFIGURATION_TYPES  
	"${CMAKE_CONFIGURATION_TYPES} lto"
)
