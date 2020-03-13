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
    uint32_t* displayPtr;
    SDL_LockTexture(texture, nullptr, (void**)&displayPtr, &pitch);
    uint32_t* ptr = displayPtr;
    for (size_t i = 0; i < buffer.size(); ++i) {
        std::copy(buffer[i].begin(), buffer[i].end(), ptr);
        ptr += buffer[i].size();
    }
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void drawEllipse(grid& buffer, Ellipse& e) {
    uint32_t bb = std::max(e.major, e.minor);

    for (size_t y = std::max(0u, e.origin.y - bb); y < std::min((uint32_t)buffer.size(), e.origin.y + bb); ++y) {
        for (size_t x = std::max(0u, e.origin.x - bb); x < std::min((uint32_t)buffer[y].size(), e.origin.x + bb); ++x) {
            int xc = (int)x - (int)e.origin.x;
            int yc = (int)y - (int)e.origin.y;
            if ((std::pow((xc * std::cos(e.angle) - yc * std::sin(e.angle)), 2) / std::pow(e.major, 2) +
                 std::pow((xc * std::sin(e.angle) + yc * std::cos(e.angle)), 2) / std::pow(e.minor, 2)) <= 1.0) {
                buffer[y][x] = 0xFF000000 | e.color;
            }
        }
    }
}

void next_generation(grid& last_gen, const uint32_t* original) {
    size_t h = last_gen.size(), w = last_gen[0].size();
    grid new_gen = last_gen;
    int current_mutation = 0;
    double current_score = 0.0, last_score = 0.0;
    bool first_hit = false;

    Ellipse e = randomEllipse(w, h);
    e.color = original[e.origin.y * w + e.origin.x];
    Ellipse best_fit = e;

    while (current_mutation < 500) {
        uint32_t bb = std::max(e.major, e.minor);
        for (size_t y = std::max(0u, e.origin.y - bb); y < std::min((uint32_t)new_gen.size(), e.origin.y + bb); ++y) {
            for (size_t x = std::max(0u, e.origin.x - bb); x < std::min((uint32_t)new_gen[y].size(), e.origin.x + bb);
                 ++x) {
                uint32_t new_color;
                int xc = (int)x - (int)e.origin.x;
                int yc = (int)y - (int)e.origin.y;
                if ((std::pow((xc * std::cos(e.angle) - yc * std::sin(e.angle)), 2) / std::pow(e.major, 2) +
                     std::pow((xc * std::sin(e.angle) + yc * std::cos(e.angle)), 2) / std::pow(e.minor, 2)) <= 1.0) {
                    new_color = 0xFF000000 | e.color;
                } else {
                    new_color = new_gen[y][x];
                }

                uint32_t og_color = original[y * w + x];
                for (size_t i = 0; i < 3; i++) {
                    int og_value = (og_color >> (i * 8)) & 0xFF;
                    int new_value = (new_color >> (i * 8)) & 0xFF;
                    int last_value = (new_gen[y][x] >> (i * 8)) & 0xFF;
                    current_score += std::abs(og_value - new_value);
                    last_score += std::abs(og_value - last_value);
                }
            }
        }

        current_score /= (bb * bb * 3);
        last_score /= (bb * bb * 3);

        if (current_score < last_score) {
            for (size_t i = 0; i < h; ++i) std::copy(last_gen[i].begin(), last_gen[i].end(), new_gen[i].begin());
            drawEllipse(new_gen, e);
            best_fit = e;
            first_hit = true;
        }
        current_score = 0;
        last_score = 0;

        e = best_fit;
        e.mutate();
        if (first_hit)
            current_mutation++;
        else {
            e = randomEllipse(w, h);
            e.color = original[e.origin.y * w + e.origin.x];
            best_fit = e;
        }
    }

    for (size_t i = 0; i < h; ++i) std::copy(new_gen[i].begin(), new_gen[i].end(), last_gen[i].begin());
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
    if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
        surface = SDL_ConvertSurface(surface, format, 0);
        SDL_FreeFormat(format);
    }
    if (surface == nullptr) {
        printf("Failed to convert image %s to RGBA32\n", file_path.c_str());
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

    bool done = false, stop_evo = false;
    size_t ctr = 0;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_Q) stop_evo = true;
        }

        if (!stop_evo && ctr < 1000) {
            next_generation(buffer, reinterpret_cast<uint32_t*>(surface->pixels));
            printf("Generation #%lu\n", ctr);
            ctr++;
        }

        render(renderer, texture, buffer);
    }

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();

    return 0;
}
