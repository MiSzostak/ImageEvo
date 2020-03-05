include(FindPkgConfig)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2 SDL2_image)

add_executable(image_evo
        Main.cpp)

target_include_directories(image_evo PUBLIC ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_libraries(image_evo PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
