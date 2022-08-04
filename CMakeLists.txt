cmake_minimum_required(VERSION 3.0.0)
project(project-hypernova)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(json-c CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2-image CONFIG REQUIRED)
find_package(SDL2-mixer CONFIG REQUIRED)
find_package(SDL2-ttf CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/conversation_node.cpp
    src/conversations.cpp
    src/main.cpp
    src/map.cpp
    src/player.cpp
    src/quests.cpp
    src/tileset.cpp
    src/world.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        json-c::json-c
        nlohmann_json nlohmann_json::nlohmann_json
        SDL2::SDL2 SDL2::SDL2main
        SDL2::SDL2_image
        SDL2::SDL2_mixer
        SDL2::SDL2_ttf
)
