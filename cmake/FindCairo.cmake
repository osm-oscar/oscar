# - Try to find the Cairo Library
# Once done this will define
#
#  CAIRO_FOUND - system has Cairo
#  CAIRO_INCLUDE_DIR - the Cairo include directory
#  CAIRO_LIBRARIES 
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

IF ( CAIRO_INCLUDE_DIR AND CAIRO_LIBRARIES )
   # in cache already
   SET( CAIRO_FIND_QUIETLY TRUE )
ENDIF ( CAIRO_INCLUDE_DIR AND CAIRO_LIBRARIES )

FIND_PATH( CAIRO_INCLUDE_DIR NAMES cairo/cairo.h )
FIND_LIBRARY( CAIRO_LIBRARIES NAMES cairo )

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( cairo DEFAULT_MSG CAIRO_INCLUDE_DIR CAIRO_LIBRARIES )
