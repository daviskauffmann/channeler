cmake_minimum_required(VERSION 3.0.0)

project(channeler)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(chlib)
add_subdirectory(chmain)
add_subdirectory(chserver)
