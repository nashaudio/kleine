# CMakeList.txt : CMake project for Kleine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

# BEFORE project(): Remove unwanted flags that VS might add
string(REPLACE "-nostartfiles" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-nostdlib" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-nostartfiles" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-nostdlib" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-nostartfiles" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
string(REPLACE "-nostdlib" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

project ("kleine" CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure we link the standard libraries
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Explicitly link standard libraries for Clang with MSVC target
    link_libraries(msvcrt vcruntime ucrt)
endif()

# Add source to this project's executable.
add_executable (kleine "kleine.cpp" "kleine.h" "tests/tree.h" "tests/unknown.h")

# disable warning C4244: 'argument': conversion from 'size_t' to 'int', possible loss of data
target_compile_options(kleine PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/wd4244 /wd4305>
)

# Disable CRT insecure warnings for MSVC compilers.
target_compile_definitions(kleine PRIVATE
    $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

# Add include directories for the project.
target_include_directories(kleine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET kleine PROPERTY CXX_STANDARD 20)
endif()