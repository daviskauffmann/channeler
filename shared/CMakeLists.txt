cmake_minimum_required(VERSION 3.0.0)
project(hypernova_shared)

set(CMAKE_C_STANDARD 99)

find_package(json-c CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2-image CONFIG REQUIRED)
find_package(SDL2-mixer CONFIG REQUIRED)
find_package(SDL2-net CONFIG REQUIRED)

add_library(${PROJECT_NAME}
    src/conversations.c
    src/map.c
    src/net.c
    src/player.c
    src/quests.c
    src/tileset.c
    src/world.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        json-c::json-c
        SDL2::SDL2 SDL2::SDL2main
        SDL2::SDL2_image
        SDL2::SDL2_mixer
        SDL2::SDL2_net
)
