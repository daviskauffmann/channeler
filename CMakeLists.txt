cmake_minimum_required(VERSION 3.0.0)
project(project-hypernova)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(shared)
