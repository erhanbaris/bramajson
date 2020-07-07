# CMakeList.txt : CMake project for bramajson, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
project(bramajson)

# Add source to this project's executable.
add_executable (bramajson "bramajson.cpp" "bramajson.h")
set_property(TARGET bramajson PROPERTY C_STANDARD 99)

# TODO: Add tests and install targets if needed.
