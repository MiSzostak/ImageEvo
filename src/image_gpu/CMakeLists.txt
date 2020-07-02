find_package(glfw3 3.3 REQUIRED)

add_executable(image_gpu main.cpp)

target_include_directories(image_gpu PUBLIC ../../libs/glad/include)

target_link_libraries(image_gpu glad glfw)
target_link_options(image_gpu PRIVATE -lGL -ldl)
