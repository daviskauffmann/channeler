cmake_minimum_required(VERSION 3.0.0)

project(chmain LANGUAGES CXX)

file(
    GLOB_RECURSE SOURCE_FILES
    SOURCES ${PROJECT_SOURCE_DIR}/**/*.cpp
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(unofficial-enet CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2-image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    chlib
    unofficial::enet::enet
    fmt::fmt
    SDL2::SDL2 SDL2::SDL2main
    SDL2::SDL2_image
    $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    spdlog::spdlog
)
