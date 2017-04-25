# - Find LibEvent (a cross platform RPC lib/tool)
# This module defines
#  LibEvent_INCLUDE_DIRS, where to find LibEvent headers
#  LibEvent_LIBRARIES, LibEvent libraries
#  LibEvent_FOUND, If false, do not try to use ant

find_path(LibEvent_INCLUDE_DIR event2/event.h PATHS "${LibEvent_Source}/include" )
if (MSVC)
	find_path(LibEvent_INCLUDE_CFG_DIR event2/event-config.h PATHS "${LibEvent_Source}/include" )
	if(LibEvent_INCLUDE_CFG_DIR)
		set(LibEvent_INCLUDE_DIR ${LibEvent_INCLUDE_DIR} ${LibEvent_INCLUDE_CFG_DIR})
	endif(LibEvent_INCLUDE_CFG_DIR)
endif(MSVC)
if(NOT LibEvent_Build AND LibEvent_Source)
	set(LibEvent_Build ${LibEvent_Source} ${LibEvent_Source}/lib )
endif()

find_library(LibEvent_LIB_Basis NAMES event PATHS ${LibEvent_Build})
find_library(LibEvent_LIB_Core NAMES event_core PATHS ${LibEvent_Build})
find_library(LibEvent_LIB_Extras NAMES event_extra PATHS ${LibEvent_Build})

if (LibEvent_LIB_Basis AND LibEvent_INCLUDE_DIR)
	set(LibEvent_FOUND TRUE)
	set(LibEvent_LIBRARIES ${LibEvent_LIB_Basis} ${LibEvent_LIB_Core} ${LibEvent_LIB_Extras} )
	set(LibEvent_INCLUDE_DIRS ${LibEvent_INCLUDE_DIR})
else ()
	set(LibEvent_FOUND FALSE)
endif ()

if (LibEvent_FOUND)
	if (NOT LibEvent_FIND_QUIETLY)
		message(STATUS "Found libevent: ${LibEvent_LIBRARIES}")
	endif ()
else ()
	message(STATUS "libevent NOT found.")
endif ()

if( MSVC )
	mark_as_advanced(
		LibEvent_INCLUDE_DIR
		LibEvent_LIB_Basis
		LibEvent_LIB_Core
		LibEvent_LIB_Extras
	)
else()
	mark_as_advanced(
		LibEvent_INCLUDE_DIR
		LibEvent_LIB_Basis
		LibEvent_LIB_Core
		LibEvent_LIB_Extras
		LibEvent_LIB_Pthreads
	)
endif()

