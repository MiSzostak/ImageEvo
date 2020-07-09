include(FindPkgConfig)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(image_gpu
        renderer.cpp
        stb_wrapper.cpp
        main.cpp)

target_include_directories(image_gpu PUBLIC ../../libs/stb ../../libs/gl3w ${SDL2_INCLUDE_DIRS})
target_link_libraries(image_gpu PRIVATE gl3w ${SDL2_LIBRARIES})
target_link_options(image_gpu PRIVATE -lGL -ldl)
