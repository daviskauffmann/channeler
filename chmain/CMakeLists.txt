cmake_minimum_required(VERSION 3.0.0)
project(chmain)

set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-enet CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2-image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/audio.cpp
    src/client.cpp
    src/display.cpp
    src/font.cpp
    src/main.cpp
    src/peer.cpp
    src/scene.cpp
    src/scenes/game_scene.cpp
    src/scenes/menu_scene.cpp
    src/sdl_image.cpp
    src/sdl_mixer.cpp
    src/sdl_ttf.cpp
    src/texture.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        chlib
        unofficial::enet::enet
        fmt::fmt
        SDL2::SDL2 SDL2::SDL2main
        SDL2::SDL2_image
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
        $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
        spdlog::spdlog
)
