cmake_minimum_required(VERSION 3.0.0)
project(chserver)

set(CMAKE_CXX_STANDARD 20)

find_package(unofficial-enet CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        chlib
        unofficial::enet::enet
        SDL2::SDL2 SDL2::SDL2main
        spdlog::spdlog
)
