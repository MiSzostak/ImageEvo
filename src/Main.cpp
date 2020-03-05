#include <SDL.h>
#include <SDL_image.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "Geometry.hpp"

using grid = std::vector<std::vector<uint32_t>>;

void render(SDL_Renderer* renderer, SDL_Texture* texture, grid& buffer) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);

    int pitch = 0;
    uint8_t* displayPtr;
    SDL_LockTexture(texture, nullptr, (void**)&displayPtr, &pitch);
    uint32_t* ptr = reinterpret_cast<uint32_t*>(displayPtr);
    for (size_t i = 0; i < buffer.size(); i++) {
        std::copy(buffer[i].begin(), buffer[i].end(), ptr);
        ptr += buffer[i].size();
    }
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void drawEllipse(grid& buffer) {
    Ellipse test(Vec2i(0, 0), 5, 3, 0);

    for (size_t y = 0; y < buffer.size(); y++) {
        for (size_t x = 0; x < buffer[y].size(); x++) {
            int xc = x - 650;
            int yc = y - 375;
            double a = 1.6;
            if ((std::pow((xc * std::cos(a) - yc * std::sin(a)), 2) / 2500.f +
                 std::pow((xc * std::sin(a) + yc * std::cos(a)), 2) / 900.f) <= 1.0) {
                buffer[y][x] &= 0xFF0000FF;
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: image_evo <image path>\n");
        return 1;
    }
    std::string file_path(argv[1]);
    std::string window_name = "ImageEvo [" + file_path + "]";

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("Failed to init image loader\n");
        return 1;
    }

    SDL_Surface* surface = IMG_Load(file_path.c_str());
    if (surface == nullptr) {
        printf("Failed to load image %s\n", file_path.c_str());
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    const int w = surface->w, h = surface->h;
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
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    // SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    // if (texture == nullptr) {
    // printf("Failed to create texture from surface\n");
    // return 1;
    // }
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (texture == nullptr) {
        printf("Failed to create texture\n");
        return 1;
    }

    grid buffer(h, std::vector<uint32_t>(w, 0xFFFFFFFF));
    drawEllipse(buffer);

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // SDL_RenderClear(renderer);
        // SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        // SDL_RenderPresent(renderer);
        render(renderer, texture, buffer);
    }

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
