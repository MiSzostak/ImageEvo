include(FindPkgConfig)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

find_package(OpenCV REQUIRED)

add_executable(image_evo
        main.cpp)

target_compile_options(image_evo PRIVATE -fopenmp)
target_link_options(image_evo PRIVATE -fopenmp)
target_include_directories(image_evo PUBLIC ../../libs/stb ${SDL2_INCLUDE_DIRS})
target_link_libraries(image_evo PUBLIC ${SDL2_LIBRARIES} ${OpenCV_LIBS})
