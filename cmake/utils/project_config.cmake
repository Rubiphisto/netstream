cmake_minimum_required( VERSION 2.8.9 )

function( netstream_add_source GROUPNAME FILENAME )
	get_filename_component( _FILENAME_EXT ${FILENAME} EXT )
	if( _FILENAME_EXT STREQUAL ".h" OR _FILENAME_EXT STREQUAL ".hpp" )
		set( HEADER_FILES ${HEADER_FILES} ${FILENAME} PARENT_SCOPE )
	elseif( _FILENAME_EXT STREQUAL ".cpp" )
		set( SOURCE_FILES ${SOURCE_FILES} ${FILENAME} PARENT_SCOPE )
	endif()
	if( GROUPNAME )
		source_group( ${GROUPNAME} FILES ${FILENAME} )
	endif()
endfunction( netstream_add_source )

function( netstream_add_definitions PROJECTNAME )
	# Warning setting
	if( MSVC )
		add_definitions( "-D_CRT_SECURE_NO_WARNINGS" )
		
		# Use the highest warning level for visual studio.
		SET( WARNING_LEVEL "/W4" )
	
		if( CMAKE_CXX_FLAGS MATCHES "/W[0-4]" )
			STRING( REGEX REPLACE "/W[0-4]" "${WARNING_LEVEL}" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
		else()
			SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_LEVEL}" )
		endif ()
	elseif( CMAKE_COMPILER_IS_GNUCXX )
		add_definitions( -Wall -Wno-long-long -pedantic ${CMAKE_CXX_FLAGS} )
	endif ()
endfunction( netstream_add_definitions )

function( netstream_add_properties PROJECTNAME )
	set_target_properties( ${PROJECTNAME} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	)
endfunction( netstream_add_properties )

function( netstream_add_define DEFINE_NAME )
	add_definitions( -D${DEFINE_NAME} )
endfunction( netstream_add_define )

function( netstream_add_dependency LIB_NAME )
	if( "${LIB_NAME}" STREQUAL "LibEvent" )
		find_package( LibEvent )
		if( LibEvent_FOUND )
			include_directories( ${LibEvent_INCLUDE_DIRS} )
			target_link_libraries( ${PROJECTNAME} ${LibEvent_LIBRARIES} )
		else()
			message( FATAL_ERROR "LibEvent isn't found." )
		endif()
	elseif( "${LIB_NAME}" STREQUAL "WinSock" )
		if( WIN32 )
			target_link_libraries( ${PROJECTNAME} ws2_32 )
		endif()
	endif()
endfunction( netstream_add_dependency )

function( netstream_app PROJECTNAME )
	message( STATUS "======== Configuring application: ${PROJECTNAME} ========" )
	# include dir
	include_directories( ${CMAKE_SOURCE_DIR}/include )
	# add source
	include( ${PROJECTNAME}.list )
	# definitions
	netstream_add_definitions( ${PROJECTNAME} )
	# project
	add_executable( ${PROJECTNAME} ${HEADER_FILES} ${SOURCE_FILES} )
	# properties
	netstream_add_properties( ${PROJECTNAME} )
	# depend netstream
	target_link_libraries( ${PROJECTNAME} netstream )
	# handle with arguments
	set( SECTIONS_GROUP "^LIBS|DEFS$" )
	if( ${ARGC} GREATER 1 )
		math( EXPR list_max_index "${ARGC}-1" )
		foreach( index RANGE 1 ${list_max_index} )
			list( GET ARGV ${index} value )
			if( "${value}" MATCHES "${SECTIONS_GROUP}" )
				string( REGEX MATCH "${SECTIONS_GROUP}" SECTION_NAME "${value}" )
			elseif( "${SECTION_NAME}" STREQUAL "LIBS" )
				netstream_add_dependency( "${value}" )
			elseif( "${SECTION_NAME}" STREQUAL "DEFS" )
				netstream_add_define( "${value}" )
			endif()
		endforeach( index )
	endif()
endfunction( netstream_app )

function( netstream_library PROJECTNAME )
	message( STATUS "======== Configuring library: ${PROJECTNAME} ========" )
	# include dir
	include_directories( ${CMAKE_SOURCE_DIR}/include )
	# add source
	include( ${PROJECTNAME}.list )
	# definitions
	netstream_add_definitions( ${PROJECTNAME} )
	# project
	if( NETSTREAM_NO_SHARED_LIB )
		add_library( ${PROJECTNAME} STATIC ${HEADER_FILES} ${SOURCE_FILES} )
	else()
		add_library( ${PROJECTNAME} SHARED ${HEADER_FILES} ${SOURCE_FILES} )
	endif()
	# properties
	netstream_add_properties( ${PROJECTNAME} )
	# handle with arguments
	set( SECTIONS_GROUP "^LIBS|DEFS$" )
	if( ${ARGC} GREATER 1 )
		math( EXPR list_max_index "${ARGC}-1" )
		foreach( index RANGE 1 ${list_max_index} )
			list( GET ARGV ${index} value )
			if( "${value}" MATCHES "${SECTIONS_GROUP}" )
				string( REGEX MATCH "${SECTIONS_GROUP}" SECTION_NAME "${value}" )
			elseif( "${SECTION_NAME}" STREQUAL "LIBS" )
				netstream_add_dependency( "${value}" )
			elseif( "${SECTION_NAME}" STREQUAL "DEFS" )
				netstream_add_define( "${value}" )
			endif()
		endforeach( index )
	endif()
endfunction( netstream_library )
