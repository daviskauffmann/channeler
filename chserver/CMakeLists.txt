cmake_minimum_required(VERSION 3.0.0)

project(chserver LANGUAGES CXX)

find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

file(
    GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp
)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME} PRIVATE
    chlib
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    spdlog::spdlog
)
