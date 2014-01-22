cmake_minimum_required (VERSION 2.8.9)

project(NETSTREAM)

# Include necessary submodules
set(CMAKE_MODULE_PATH
	"${NETSTREAM_SOURCE_DIR}/CMake"
	"${NETSTREAM_SOURCE_DIR}/CMake/Utils"
	"${NETSTREAM_SOURCE_DIR}/CMake/Packages"
)

add_subdirectory(src)
add_subdirectory(test)
