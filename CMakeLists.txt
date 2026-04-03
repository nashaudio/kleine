# CMakeList.txt : CMake project for Kleine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("kleine")

# Add source to this project's executable.
add_executable (kleine "kleine.cpp" "kleine.h")

# disable warning C4244: 'argument': conversion from 'size_t' to 'int', possible loss of data
target_compile_options(kleine PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/wd4244 /wd4305>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-error=changes-meaning>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-error=deprecated>
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
  set_property(TARGET kleine PROPERTY CXX_STANDARD 17)
endif()

# TODO: Add tests and install targets if needed.
