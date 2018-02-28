IF ( (NOT LTO_PLUGIN_PARAMETERS_SET) OR (CMAKE_GCC_VERSION_FOR_LTO))
	IF(NOT CMAKE_GCC_VERSION_FOR_LTO)
		SET(CMAKE_GCC_VERSION_FOR_LTO
			$ENV{CMAKE_GCC_VERSION_FOR_LTO}
			CACHE STRING "Gcc version specified on command line during initial cache population"
			FORCE
		)
	ENDIF()

	if (NOT CMAKE_GCC_VERSION_FOR_LTO)
		message(FATAL_ERROR "You have to set CMAKE_GCC_VERSION_FOR_LTO to a proper value before running cmake. Set to disable to disable configuration.")
	endif()
	
	if ("${CMAKE_GCC_VERSION_FOR_LTO}" MATCHES "disable")
		message(STATUS "LTO Setup: Will not try to configure lto. You have to provide proper gcc, nm, ar on your own.")
	else()
		SET(CMAKE_C_COMPILER
			/usr/bin/gcc-${CMAKE_GCC_VERSION_FOR_LTO}
			CACHE STRING ""
			FORCE
		)

		set(CMAKE_CXX_COMPILER
			"/usr/bin/g++-${CMAKE_GCC_VERSION_FOR_LTO}"
			CACHE STRING ""
			FORCE
		)

		SET(CMAKE_AR
			"/usr/bin/gcc-ar-${CMAKE_GCC_VERSION_FOR_LTO}"
			CACHE STRING ""
			FORCE
		)

		SET(CMAKE_NM
			"/usr/bin/gcc-nm-${CMAKE_GCC_VERSION_FOR_LTO}"
			CACHE STRING ""
			FORCE
		)

		SET(CMAKE_RANLIB
			"/usr/bin/gcc-ranlib-${CMAKE_GCC_VERSION_FOR_LTO}"
			CACHE STRING ""
			FORCE
		)

		SET(CMAKE_LTO_PLUGIN_PATH
			"/usr/lib/gcc/x86_64-linux-gnu/${CMAKE_GCC_VERSION_FOR_LTO}/liblto_plugin.so"
			CACHE INTERNAL ""
			FORCE
		)

		SET(CMAKE_C_ARCHIVE_CREATE
			"<CMAKE_AR> qcs --plugin ${CMAKE_LTO_PLUGIN_PATH} <TARGET> <OBJECTS>"
			CACHE INTERNAL ""
			FORCE
		)

		SET(CMAKE_CXX_ARCHIVE_CREATE
			"<CMAKE_AR> qcs --plugin ${CMAKE_LTO_PLUGIN_PATH} <TARGET> <OBJECTS>"
			CACHE INTERNAL ""
			FORCE
		)
	endif()
	
	SET(LTO_PLUGIN_PARAMETERS_SET
		"True"
		CACHE INTERNAL ""
		FORCE
	)
ENDIF()
