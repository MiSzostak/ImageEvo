#include "ellipse.h"

__global__ void draw_ellipse_inefficient(u8* buffer, u32 size, int width, int height, u32 origin_x,
                                         u32 origin_y,int major, int minor, double angle, u8* image) {
    // prepare constants
    const double angle_cos = cos(angle);
    const double angle_sin = sin(angle);
    const double pow_major = pow(major, 2);
    const double pow_minor = pow(minor, 2);
    const u32 color_start = (origin_x + origin_y * width) * 3;

    // setup indexing
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    // grid stride loop
    int stride = blockDim.x * gridDim.x;

    for (int i = index; i < size; i += stride) {
        // get 2d coordinates
        const int x = (i / 3) % width;
        const int y = (i / 3) / width;
        const int xc = x - int(origin_x);
        const int yc = y - int(origin_y);

        if ((pow((xc * angle_cos - yc * angle_sin), 2) / pow_major +
             pow((xc * angle_sin + yc * angle_cos), 2) / pow_minor) <= 1.0) {
            buffer[index] = image[color_start + (index % 3)];
        }
    }
}

__global__ void calc_fitness_inefficient(u8* buffer, u32 size, int width, int height, u32 origin_x,
                                         u32 origin_y,int major, int minor, double angle, u32 color,
                                         u8* image, u8* new_gen, FitCalcResult* result) {
    // prepare constants
    const double angle_cos = cos(angle);
    const double angle_sin = sin(angle);
    const double pow_major = pow(major, 2);
    const double pow_minor = pow(minor, 2);

    // setup indexing
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    // grid stride loop
    int stride = blockDim.x * gridDim.x;

    for (int i = index; i < size; i += stride) {
        // get 2d coordinates
        const int x = (i / 3) % width;
        const int y = (i / 3) / width;
        const int xc = x - int(origin_x);
        const int yc = y - int(origin_y);

        u8 new_color;
        if ((pow((xc * angle_cos - yc * angle_sin), 2) / pow_major +
             pow((xc * angle_sin + yc * angle_cos), 2) / pow_minor) <= 1.0) {
            new_color = color >> ((index % 3) * 8);
        } else {
            new_color = new_gen[index];
        }

        result->new_score += abs(int(image[index]) - int(new_color));
        result->old_score += abs(int(image[index]) - int(new_gen[index]));
    }
}

void AllocBuffer(void** pointer, u32 size) {
    cudaMallocManaged(pointer, size);
}

void FreeBuffer(void** pointer) {
    cudaFree(pointer);
}

void DrawEllipseGPU(u8* buffer, u32 size, int width, int height, u32 origin_x, u32 origin_y,
                    int major, int minor, double angle, u8* image) {
    int blockSize = 256;
    int numBlocks = (size + blockSize - 1) / blockSize;
    draw_ellipse_inefficient<<<numBlocks, blockSize>>>(
        buffer, size, width, height, origin_x, origin_y, major, minor, angle, image);
    cudaDeviceSynchronize();
}

void CalcFitnessGPU(u8* buffer, u32 size, int width, int height, u32 origin_x,
                              u32 origin_y,int major, int minor, double angle, u32 color,
                              u8* image, u8* new_gen, FitCalcResult* result) {
    int blockSize = 256;
    int numBlocks = (size + blockSize - 1) / blockSize;
    calc_fitness_inefficient<<<numBlocks, blockSize>>>(
        buffer, size, width, height, origin_x, origin_y, major, minor, angle, color, image,
        new_gen, result);
    cudaDeviceSynchronize();
}