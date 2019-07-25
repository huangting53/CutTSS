cmake_minimum_required(VERSION 3.13)
project(CutTSS_OVS)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(OVS)

add_executable(CutTSS_OVS
        OVS/cmap.cpp
        OVS/cmap.h
        OVS/ElementaryClasses.h
        OVS/hash.h
        OVS/MapExtensions.cpp
        OVS/MapExtensions.h
        OVS/random.h
        OVS/TupleSpaceSearch.cpp
        OVS/TupleSpaceSearch.h
        CutTSS.cpp
        CutTSS.h
        main.cpp)
