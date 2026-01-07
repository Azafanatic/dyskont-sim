cmake_minimum_required(VERSION 3.10)
project(dyskont-sim)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER "gcc")

include_directories(include)

add_executable(kierownik
		src/kierownik.c
		src/dyskont_utils.c
)

add_executable(klient
		src/klient.c
		src/dyskont_utils.c
)

add_executable(logger
		src/logger.c
		src/dyskont_utils.c
)

add_executable(dyskont
		src/main.c
		src/dyskont_utils.c
)

add_executable(kasa_samoobslugowa
		src/kasa_samoobslugowa.c
		src/dyskont_utils.c
)

add_executable(kasa_stacjonarna
		src/kasa_stacjonarna.c
		src/dyskont_utils.c
)

add_executable(obsluga
		src/obsluga.c
		src/dyskont_utils.c
)
