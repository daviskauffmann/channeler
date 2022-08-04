cmake_minimum_required(VERSION 3.0.0)
project(project-hypernova)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(shared)
