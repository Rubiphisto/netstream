cmake_minimum_required (VERSION 2.8.9)

set (PROJECTNAME demo_server)

project(${PROJECTNAME})

include(${PROJECTNAME}.list)

if (MSVC)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif (MSVC)

include_directories(
	${CMAKE_SOURCE_DIR}/include
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

#add_executable(${PROJECTNAME} ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)
add_executable(${PROJECTNAME} ${HEADER_FILES} ${SOURCE_FILES})

if (MSVC)
	target_link_libraries(demo_server netstream)
elseif (UNIX)
	target_link_libraries(demo_server netstream)
endif ()

# 

set (PROJECTNAME demo_client)

project(${PROJECTNAME})

include(${PROJECTNAME}.list)

if (MSVC)
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif (MSVC)

include_directories(
	${CMAKE_SOURCE_DIR}/include
)

link_directories(${CMAKE_SOURCE_DIR}/lib)
#add_executable(${PROJECTNAME} ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)
add_executable(${PROJECTNAME} ${HEADER_FILES} ${SOURCE_FILES})

if (MSVC)
	target_link_libraries(${PROJECTNAME} netstream)
elseif (UNIX)
	target_link_libraries(${PROJECTNAME} netstream)
endif ()
