cmake_minimum_required (VERSION 2.8.9)

set (PROJECTNAME netstream)

project(${PROJECTNAME})

include(${PROJECTNAME}.list)

add_definitions(-DLIBNETSTREAM_DLL)
add_definitions(-DLIBNETSTREAM_EXPORT)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Warning setting
if (MSVC)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	
	# Use the highest warning level for visual studio.
	SET(WARNING_LEVEL "/W4")

	if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		STRING(REGEX REPLACE "/W[0-4]" "${WARNING_LEVEL}" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else ()
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_LEVEL}")
	endif ()
elseif(CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-Wall -Wno-long-long -pedantic ${CMAKE_CXX_FLAGS})
endif ()

if( NETSTREAM_NO_SHARED_LIB )
	add_library(${PROJECTNAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})
else()
	add_library(${PROJECTNAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

find_package(LibEvent)

if(LibEvent_FOUND)
	include_directories(${LibEvent_INCLUDE_DIRS})
	target_link_libraries(${PROJECTNAME} ${LibEvent_LIBRARIES})
endif()
 
if(WIN32)
	target_link_libraries(${PROJECTNAME} ws2_32)
endif(WIN32)

install( TARGETS ${PROJECTNAME} DESTINATION lib )
install( FILES ../include/libnetstream.h DESTINATION include )

