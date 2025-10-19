cmake_minimum_required(VERSION 3.10)

project(client244 C)

set(PLATFORM sdl3 CACHE STRING "Platform to build with")
set_property(CACHE PLATFORM PROPERTY STRINGS sdl3 sdl2)

if(PLATFORM STREQUAL "sdl3")
    find_package(SDL3 REQUIRED)
    set(platform_directories platform/sdl3)
    set(platform_libraries ${SDL3_LIBRARIES})
    set(platform_sources platform/sdl3/platform.c)
endif()

add_executable(client244 ${platform_sources} client.c pix2d.c pix3d.c world.c world3d.c bzip2.c packet.c file_stream.c jagfile.c pix_font.c pix32.c pix_map.c pix8.c lib/math.c lib/jpeg.c lib/util.c)

target_include_directories(client244 PRIVATE ${platform_directories})
target_link_libraries(client244 PRIVATE ${platform_libraries})
