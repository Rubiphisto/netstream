cmake_minimum_required (VERSION 2.8.9)

project(NETSTREAM CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG( -std=c++11 COMPILER_SUPPORTS_CXX11 )
message( STATUS "| ${CMAKE_CXX_FLAGS} | ${CMAKE_CXX_COMPILER}" )

if( COMPILER_SUPPORTS_CXX11 )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
else()
	message( FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler." )
endif()

# Include necessary submodules
set(CMAKE_MODULE_PATH
	"${NETSTREAM_SOURCE_DIR}/cmake"
	"${NETSTREAM_SOURCE_DIR}/cmake/Utils"
	"${NETSTREAM_SOURCE_DIR}/cmake/Packages"
)

add_subdirectory(src)
add_subdirectory(test)

