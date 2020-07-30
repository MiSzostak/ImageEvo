#include <SDL.h>

#include <cstdio>
#include <string>
#include <cstring>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

#include "ellipse.h"
#include "geometry.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
void Render(SDL_Renderer* renderer, SDL_Texture* texture, u8* buffer, u32 size) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);

    int pitch = 0;
    u8* displayPtr = nullptr;
    SDL_LockTexture(texture, nullptr, (void**)&displayPtr, &pitch);
    std::memcpy(displayPtr, buffer, size);
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void NextGeneration(int width, int height, u8* scratch_buffer, u8* out_buffer, u8* original, FitCalcResult* result) {
    const u32 size = width * height * 3;
    std::memcpy(scratch_buffer, out_buffer, size);
    int current_mutation = 0;
    result->new_score = 0.0;
    result->old_score = 0.0;
    bool first_hit = false;

    Ellipse e = RandomEllipse(width, height);
    e.color.r = original[(e.origin.y * width + e.origin.x) * 3];
    e.color.g = original[(e.origin.y * width + e.origin.x) * 3 + 1];
    e.color.b = original[(e.origin.y * width + e.origin.x) * 3 + 2];
    Ellipse best_fit = e;

    while (current_mutation < 500) {
        CalcFitnessGPU(out_buffer, size, width, height, e.origin.x, e.origin.y, e.major, e.minor, e.angle,
                       e.color.ToU32(), original, scratch_buffer, result);

        result->new_score /= size;
        result->old_score /= size;

        if (result->new_score < result->old_score) {
            std::memcpy(scratch_buffer, out_buffer, size);
            DrawEllipseGPU(scratch_buffer, size, width, height, e.origin.x, e.origin.y, e.major, e.minor,
                           e.angle, original);
            best_fit = e;
            first_hit = true;
        }
        result->new_score = 0;
        result->old_score = 0;

        e = best_fit;
        e.Mutate(width, height);
        if (first_hit) {
            current_mutation++;
        } else {
            e = RandomEllipse(width, height);
            e.color.r = original[(e.origin.y * width + e.origin.x) * 3];
            e.color.g = original[(e.origin.y * width + e.origin.x) * 3 + 1];
            e.color.b = original[(e.origin.y * width + e.origin.x) * 3 + 2];
            best_fit = e;
        }
    }

    std::memcpy(out_buffer, scratch_buffer, size);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("No image specified\n");
        return 1;
    }

    std::string file_path(argv[1]);
    int gen_limit = 500;
    bool headless = true;

    if (gen_limit < 0) {
        printf("Invalid generation limit %d\n", gen_limit);
        return 1;
    }

    std::vector<u8> buffer;
    int w, h, c;
    u8* raw_image_data = stbi_load(file_path.c_str(), &w, &h, &c, 0);
    std::vector<u8> image(w * h * 3);
    std::memcpy(image.data(), raw_image_data, image.size());

    if (!image.data() || c != 3) {
        printf("Failed to load image %s\n", file_path.c_str());
        return 1;
    }

    u8* uni_buffer = nullptr;
    AllocBuffer(reinterpret_cast<void**>(&uni_buffer), image.size());
    if (!uni_buffer) {
        printf("Failed to allocate unified buffer\n");
        return 1;
    }

    u8* scratch_buffer = nullptr;
    AllocBuffer(reinterpret_cast<void**>(&scratch_buffer), image.size());
    if (!scratch_buffer) {
        printf("Failed to allocate unified buffer\n");
        return 1;
    }

    FitCalcResult* result = nullptr;
    AllocBuffer(reinterpret_cast<void**>(&result), sizeof(FitCalcResult));
    if (!result) {
        printf("Failed to allocate unified buffer\n");
        return 1;
    }

    u8* image_buffer = nullptr;
    AllocBuffer(reinterpret_cast<void**>(&image_buffer), image.size());
    if (!image_buffer) {
        printf("Failed to allocate unified buffer\n");
        return 1;
    }
    std::memcpy(image_buffer, raw_image_data, w * h * 3);

    int gen_ctr = 0;

    if (headless) {
        while (gen_ctr < gen_limit) {
            NextGeneration(w, h, scratch_buffer, uni_buffer, image_buffer, result);
            printf("Generation #%d\n", gen_ctr);
            gen_ctr++;
        }
        u64 start = file_path.find_last_of('/');
        std::string out_file = "out_" + file_path.substr(start == std::string::npos ? 0 : start);
        printf("Finished after %d generations, saving to %s\n", gen_ctr, out_file.c_str());
        stbi_write_png(out_file.c_str(), w, h, c, buffer.data(), 0);
        return 0;
    }

    std::string window_name = "ImageEvo [" + file_path + "]";
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow(window_name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        printf("Failed to init window\n");
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        printf("Failed to init renderer\n");
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (texture == nullptr) {
        printf("Failed to create texture\n");
        return 1;
    }

    printf("Press H to pause and S to save to file\n");
    bool done = false, pause = false, take_screenshot = false;
    bool draw_test = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_H) pause = !pause;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_S) take_screenshot = true;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_T) draw_test = true;
        }

        if (take_screenshot) {
            take_screenshot = false;
            printf("Saved screenshot\n");
            stbi_write_png("screenshot.png", w, h, c, buffer.data(), 0);
        }

        if (!pause && gen_ctr < gen_limit) {
            NextGeneration(w, h, scratch_buffer, uni_buffer, image_buffer, result);
            printf("Generation #%d\n", gen_ctr);
            gen_ctr++;
        }

        //if (draw_test) {
        //    Ellipse e = RandomEllipse(w, h);
        //    DrawEllipseGPU(uni_buffer, image.size(), w, h,
        //                   e.origin.x, e.origin.y, e.major, e.minor, e.angle, image_buffer);
        //    draw_test = false;
        //    printf("Draw\n");
        //}

        Render(renderer, texture, uni_buffer, image.size());
    }

    FreeBuffer(reinterpret_cast<void**>(&uni_buffer));
    FreeBuffer(reinterpret_cast<void**>(&scratch_buffer));
    FreeBuffer(reinterpret_cast<void**>(&image_buffer));
    FreeBuffer(reinterpret_cast<void**>(&result));
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

#pragma clang diagnostic pop