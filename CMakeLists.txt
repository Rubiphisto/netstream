cmake_minimum_required (VERSION 2.8.9)

project(NETSTREAM CXX)

if( NETSTREAM_NO_SHARED_LIB )
	message( STATUS "Build netstream as static library.")
	set( BUILD_SHARED_LIBS OFF )
else()
	message( STATUS "Build netstream as shared lib.")
	set( BUILD_SHARED_LIBS ON )
endif()

if( MSVC )
	message( STATUS "The compiler is Visual Studio." )
else()
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG( -std=c++11 COMPILER_SUPPORTS_CXX11 )
	message( STATUS "| ${CMAKE_CXX_FLAGS} | ${CMAKE_CXX_COMPILER}" )

	if( COMPILER_SUPPORTS_CXX11 )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
	else()
		message( FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler." )
	endif()
endif()

# Include necessary submodules
set(CMAKE_MODULE_PATH
	"${NETSTREAM_SOURCE_DIR}/cmake"
	"${NETSTREAM_SOURCE_DIR}/cmake/Packages"
)

add_subdirectory(src)
add_subdirectory(test)

