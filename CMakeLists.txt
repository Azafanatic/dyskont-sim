cmake_minimum_required(VERSION 3.10)
project(dyskont-sim)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER "gcc")

include_directories(include)

add_executable(manager
		src/manager.c
		src/utils.c
)
target_link_libraries(manager m)

add_executable(client
		src/client.c
		src/utils.c
)
target_link_libraries(client m)

add_executable(logger
		src/logger.c
		src/utils.c
)

add_executable(sim
		src/main.c
		src/utils.c
)

add_executable(self_service_checkout
		src/self_service_checkout.c
		src/utils.c
)

add_executable(checkout
		src/checkout.c
		src/utils.c
)

add_executable(staff
		src/staff.c
		src/utils.c
)
