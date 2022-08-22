cmake_minimum_required(VERSION 3.0.0)

project(chlib LANGUAGES CXX)

file(
    GLOB_RECURSE SOURCE_FILES
    SOURCES ${PROJECT_SOURCE_DIR}/**/*.cpp
)
add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(unofficial-enet CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    unofficial::enet::enet
    nlohmann_json nlohmann_json::nlohmann_json
    SDL2::SDL2 SDL2::SDL2main
    spdlog::spdlog
)
