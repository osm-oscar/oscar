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
		if (NOT EXISTS "${CMAKE_LTO_PLUGIN_PATH}")
			message(WARNING "Since you did not set CMAKE_LTO_PLUGIN_PATH you also have to set CMAKE_C_ARCHIVE_CREATE and CMAKE_CXX_ARCHIVE_CREATE")
		endif()
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
		
		if (NOT EXISTS "${CMAKE_LTO_PLUGIN_PATH}")
			message(STATUS "Trying to find lto plugin path to set CMAKE_LTO_PLUGIN_PATH")
			find_path(CMAKE_LTO_PLUGIN_DIR
				"liblto_plugin.so"
				"liblto_plugin.so.0"
				"liblto_plugin.so.0.0"
				"liblto_plugin.so.0.0.0"
				"liblto_plugin.so.${CMAKE_GCC_VERSION_FOR_LTO}"
				HINTS
				"/usr/lib/gcc/x86_64-linux-gnu/${CMAKE_GCC_VERSION_FOR_LTO}"
				"/usr/lib/gcc/x86_64-pc-linux-gnu/${CMAKE_GCC_VERSION_FOR_LTO}"
				"/usr/libexec/gcc/x86_64-pc-linux-gnu/${CMAKE_GCC_VERSION_FOR_LTO}"
			)
			SET(CMAKE_LTO_PLUGIN_PATH
				"${CMAKE_LTO_PLUGIN_DIR}/liblto_plugin.so"
				CACHE STRING "path to gcc liblto plugin"
				FORCE
			)
        endif()
		if(NOT EXISTS "${CMAKE_LTO_PLUGIN_PATH}")
            message(FATAL_ERROR "CMAKE_LTO_PLUGIN_PATH not set")
		else()
			message(STATUS "CMAKE_LTO_PLUGIN_PATH: ${CMAKE_LTO_PLUGIN_PATH}")
		endif()
	endif()
	
	if (EXISTS "${CMAKE_LTO_PLUGIN_PATH}")
		SET(CMAKE_C_ARCHIVE_CREATE
			"<CMAKE_AR> rcs --plugin ${CMAKE_LTO_PLUGIN_PATH} <TARGET> <OBJECTS>"
			CACHE INTERNAL ""
			FORCE
		)

		SET(CMAKE_CXX_ARCHIVE_CREATE
			"<CMAKE_AR> rcs --plugin ${CMAKE_LTO_PLUGIN_PATH} <TARGET> <OBJECTS>"
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
