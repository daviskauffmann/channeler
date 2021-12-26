cmake_minimum_required(VERSION 3.0.0)
project(client)

set(CMAKE_C_STANDARD 99)

find_package(json-c CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2-image CONFIG REQUIRED)
find_package(SDL2-mixer CONFIG REQUIRED)
find_package(SDL2-net CONFIG REQUIRED)
find_package(SDL2-ttf CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.c
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        shared
        json-c::json-c
        SDL2::SDL2 SDL2::SDL2main
        SDL2::SDL2_image
        SDL2::SDL2_mixer
        SDL2::SDL2_net
        SDL2::SDL2_ttf
)
