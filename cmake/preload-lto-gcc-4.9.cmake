set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_GCC_VERSION_FOR_LTO
	"4.9"
	CACHE INTERNAL ""
	FORCE
)

include(${CMAKE_SCRIPT_MODE_FILE}/cmake/preload-lto-gcc.cmake)

IF (NOT LTO_PLUGIN_PARAMETERS_SET)
	message(FATAL_ERROR "Configuring lto paramters failed")
ENDIF(NOT LTO_PLUGIN_PARAMETERS_SET)