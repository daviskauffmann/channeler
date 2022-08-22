cmake_minimum_required(VERSION 3.0.0)

project(chserver LANGUAGES CXX)

file(
    GLOB_RECURSE SOURCE_FILES
    SOURCES ${PROJECT_SOURCE_DIR}/**/*.cpp
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    chlib
    SDL2::SDL2 SDL2::SDL2main
    spdlog::spdlog
)
