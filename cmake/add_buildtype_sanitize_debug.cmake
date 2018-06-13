cmake_minimum_required(VERSION 3.0)

SET(CMAKE_CXX_FLAGS_SANITIZEDEBUG
	"-g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer"
	CACHE STRING "Flags used by the C++ compiler during SanitizeDebug builds."
	FORCE
)
SET(CMAKE_C_FLAGS_SANITIZEDEBUG
	"-g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer"
	CACHE STRING "Flags used by the C compiler during SanitizeDebug builds."
	FORCE
)
SET(CMAKE_EXE_LINKER_FLAGS_SANITIZEDEBUG
	""
	CACHE STRING "Flags used for linking binaries during SanitizeDebug builds."
	FORCE
)
SET(CMAKE_SHARED_LINKER_FLAGS_SANITIZEDEBUG
	""
	CACHE STRING "Flags used by the shared libraries linker during SanitizeDebug builds."
	FORCE
)

MARK_AS_ADVANCED(
	CMAKE_CXX_FLAGS_SANITIZEDEBUG
	CMAKE_C_FLAGS_SANITIZEDEBUG
	CMAKE_EXE_LINKER_FLAGS_SANITIZEDEBUG
	CMAKE_SHARED_LINKER_FLAGS_SANITIZEDEBUG
)

set(CMAKE_CONFIGURATION_TYPES  
	"${CMAKE_CONFIGURATION_TYPES} SanitizeDebug"
)
