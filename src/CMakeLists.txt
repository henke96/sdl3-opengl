cmake_minimum_required(VERSION 3.16)

project(sdl3-opengl C)

add_executable(sdl3-opengl sdl3-opengl.c gl.c)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GLESV2 glesv2)
endif()
if(NOT GLESV2_FOUND)
    find_package(GLESV2)
endif()
if(GLESV2_FOUND)
    target_include_directories(sdl3-opengl PRIVATE ${GLESV2_INCLUDE_DIRS})
endif()

find_package(SDL3 REQUIRED)
target_link_libraries(sdl3-opengl PRIVATE ${SDL3_LIBRARIES})
