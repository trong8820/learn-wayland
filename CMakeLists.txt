cmake_minimum_required(VERSION 3.21)

project(learn-wayland VERSION 1.0.0)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_UNITY_BUILD ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME}
	${CMAKE_SOURCE_DIR}/src/xdg-shell.c
	${CMAKE_SOURCE_DIR}/src/main.c
)
target_link_libraries(${PROJECT_NAME} PRIVATE wayland-client)
