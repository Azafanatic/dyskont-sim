cmake_minimum_required(VERSION 3.10)
project(dyskont-sim)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER "gcc")

include_directories(include)

add_executable(dyskont
		src/main.c
		src/kierownik.c
)
