cmake_minimum_required(VERSION 3.18)
project(HGL-SQLite3Wrapper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
unset(__BUILD_TYPE)

include_directories(include)


aux_source_directory(src Srcs)
add_library(hgsqlite3w SHARED ${Srcs})
target_compile_definitions(hgsqlite3w PRIVATE _HGL_BUILD_DL)
target_link_libraries(hgsqlite3w PRIVATE sqlite3)
unset(Srcs)


add_subdirectory(test)
