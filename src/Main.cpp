#include <SDL.h>
#include <SDL_image.h>

#include <cstdio>
#include <string>

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

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        printf("Failed to init renderer\n");
        return 1;
    }
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) {
        printf("Failed to create texture from surface\n");
        return 1;
    }
    SDL_FreeSurface(surface);

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
