include(FindPkgConfig)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

enable_language(CUDA)
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -gencode;arch=compute_75,code=sm_75;")

add_executable(image_evo_gpu
        ellipse.cu
        stb_wrapper.cpp
        main.cpp)

target_include_directories(image_evo_gpu PUBLIC ../../libs/stb ${SDL2_INCLUDE_DIRS})
target_link_libraries(image_evo_gpu PUBLIC ${SDL2_LIBRARIES})
