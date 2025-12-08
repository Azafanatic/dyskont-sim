cmake_minimum_required(VERSION 3.10)
project(dyskont-sim)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER "gcc")

include_directories(include)

add_executable(kierownik
		src/kierownik.c
		src/wiadomosc.c
)

add_executable(klient
		src/klient.c
		src/wiadomosc.c
)

add_executable(dyskont
		src/main.c
		src/wiadomosc.c
)
