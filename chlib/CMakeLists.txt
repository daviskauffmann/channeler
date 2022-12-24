cmake_minimum_required(VERSION 3.0.0)

project(chlib LANGUAGES CXX)

file(
    GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp
)
add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

find_package(unofficial-enet CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
    unofficial::enet::enet
    nlohmann_json nlohmann_json::nlohmann_json
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    spdlog::spdlog
)
