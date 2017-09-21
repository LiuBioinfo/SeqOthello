cmake_minimum_required(VERSION 2.8)
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

PROJECT(SeqOthello)
find_package (Threads)
find_package (ZLIB REQUIRED )
add_definitions(-std=gnu++11)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(src bin)

