cmake_minimum_required(VERSION 3.10)
project(dyskont-sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER "g++")

include_directories(include)

add_executable(dyskont
		src/main.cpp
		src/kierownik.cpp
)
