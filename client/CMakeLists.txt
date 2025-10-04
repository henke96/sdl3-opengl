cmake_minimum_required(VERSION 3.10)

project(client C)

set(PLATFORM sdl3 CACHE STRING "Platform to build with")
set_property(CACHE PLATFORM PROPERTY STRINGS sdl3 sdl2)

if(PLATFORM STREQUAL "sdl3")
    find_package(SDL3 REQUIRED)
    set(platform_directories platform/sdl3)
    set(platform_libraries ${SDL3_LIBRARIES})
    set(platform_sources platform/sdl3/platform.c)
endif()

add_executable(client ${platform_sources} client.c util.c pix2d.c pix3d.c world.c world3d.c bzip2.c packet.c file_stream.c jagfile.c pix_font.c jpeg.c pix32.c pix_map.c pix8.c math.c)

target_include_directories(client PRIVATE ${platform_directories})
target_link_libraries(client PRIVATE ${platform_libraries})
