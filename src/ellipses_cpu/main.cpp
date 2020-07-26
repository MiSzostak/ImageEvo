#include <SDL.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <omp.h>
#include <stb_image_write.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "geometry.hpp"

void Render(SDL_Renderer* renderer, SDL_Texture* texture, std::vector<u8>& buffer) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);

    int pitch = 0;
    u8* displayPtr = nullptr;
    SDL_LockTexture(texture, nullptr, (void**)&displayPtr, &pitch);
    std::memcpy(displayPtr, buffer.data(), buffer.size());
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void DrawEllipse(int width, int height, std::vector<u8>& buffer, Ellipse& e) {
    const int bb = std::max(e.major, e.minor);
    const auto start_y = std::max<u32>(0, e.origin.y - bb), end_y = std::min<u32>(height, e.origin.y + bb);
    const auto start_x = std::max<u32>(0, e.origin.x - bb), end_x = std::min<u32>(width, e.origin.x + bb);

    const double cos = std::cos(e.angle), sin = std::sin(e.angle);
    const double pow_major = std::pow(e.major, 2), pow_minor = std::pow(e.minor, 2);

    #pragma omp parallel for collapse(2)
    for (u32 y = start_y; y < end_y; ++y) {
        for (u32 x = start_x; x < end_x; ++x) {
            int xc = (int)x - (int)e.origin.x;
            int yc = (int)y - (int)e.origin.y;
            if ((std::pow((xc * cos - yc * sin), 2) / pow_major +
                 std::pow((xc * sin + yc * cos), 2) / pow_minor) <= 1.0) {
                u32 index = (x + y * width) * 3;
                buffer[index] = e.color.r;
                buffer[index + 1] = e.color.g;
                buffer[index + 2] = e.color.b;
            }
        }
    }
}

void NextGeneration(int width, int height, std::vector<u8>& last_gen, const u8* original) {
    std::vector<u8> new_gen = last_gen;
    int current_mutation = 0;
    double current_score = 0.0, last_score = 0.0;
    bool first_hit = false;

    Ellipse e = RandomEllipse(width, height);
    e.color.r = original[(e.origin.y * width + e.origin.x) * 3];
    e.color.g = original[(e.origin.y * width + e.origin.x) * 3 + 1];
    e.color.b = original[(e.origin.y * width + e.origin.x) * 3 + 2];
    Ellipse best_fit = e;

    while (current_mutation < 500) {
        const int bb = std::max(e.major, e.minor);
        const auto start_y = std::max<u32>(0, e.origin.y - bb), end_y = std::min<u32>(height, e.origin.y + bb);
        const auto start_x = std::max<u32>(0, e.origin.x - bb), end_x = std::min<u32>(width, e.origin.x + bb);

        const double cos = std::cos(e.angle), sin = std::sin(e.angle);
        const double pow_major = std::pow(e.major, 2), pow_minor = std::pow(e.minor, 2);

        #pragma omp parallel for collapse(2) reduction(+:current_score, last_score)
        for (u32 y = start_y; y < end_y; ++y) {
            for (u32 x = start_x; x < end_x; ++x) {
                Color new_color;
                const int xc = (int)x - (int)e.origin.x;
                const int yc = (int)y - (int)e.origin.y;
                const u32 index = (x + y * width) * 3;
                if ((std::pow((xc * cos - yc * sin), 2) / pow_major +
                     std::pow((xc * sin + yc * cos), 2) / pow_minor) <= 1.0) {
                    new_color = e.color;
                } else {
                    new_color.r = new_gen[index];
                    new_color.g = new_gen[index + 1];
                    new_color.b = new_gen[index + 2];
                }

                current_score += std::abs(int(original[index]) - int(new_color.r));
                current_score += std::abs(int(original[index + 1]) - int(new_color.g));
                current_score += std::abs(int(original[index + 2]) - int(new_color.b));

                last_score += std::abs(int(original[index]) - int(new_gen[index]));
                last_score += std::abs(int(original[index + 1]) - int(new_gen[index + 1]));
                last_score += std::abs(int(original[index + 2]) - int(new_gen[index + 2]));
            }
        }

        current_score /= (bb * bb * 3);
        last_score /= (bb * bb * 3);

        if (current_score < last_score) {
            std::memcpy(new_gen.data(), last_gen.data(), last_gen.size());
            DrawEllipse(width, height, new_gen, e);
            best_fit = e;
            first_hit = true;
        }
        current_score = 0;
        last_score = 0;

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

    std::memcpy(last_gen.data(), new_gen.data(), new_gen.size());
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) {
        printf("Usage: image_evo <image path> [generation limit]\n");
        return 1;
    }
    std::string file_path(argv[1]);
    std::string window_name = "ImageEvo [" + file_path + "]";

    int gen_limit = argc == 3 ? std::atoi(argv[2]) : 1000;
    if (gen_limit < 0) {
        printf("Invalid generation limit %d\n", gen_limit);
        return 1;
    }

    cv::Mat image = cv::imread(file_path);
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
    const int w = image.cols, h = image.rows, components = image.channels();

    if (!image.data || components != 3) {
        printf("Failed to load image %s\n", file_path.c_str());
        return 1;
    }

    // create a blurred background
    cv::Mat result;
    cv::medianBlur(image, result, 151);

    // TODO: try black edges with thicker lines
#if 0
    cv::Mat grayscale, edges, result;
    result.create(image.size(), image.type());
    // create grayscale image
    cv::cvtColor(image, grayscale, cv::COLOR_RGB2GRAY);
    // smooth image for edge detection
    cv::blur(grayscale, edges, cv::Size(3, 3));
    // sobel filter
    cv::Canny(edges, edges, 0, 150, 3);
    // start with black background
    //result = cv::Scalar::all(0);
    result = blur.clone();
    // copy only the edges into result (use edges matrix as a mask)
    image.copyTo(result, edges);
#endif

    //stbi_write_png("sobel.png", w, h, 3, result.data, 0);
    stbi_write_png("blur.png", w, h, 3, result.data, 0);

    // use the heavily blurred image as the baseline
    std::vector<u8> buffer(w * h * 3);
    std::memcpy(buffer.data(), result.data, buffer.size());

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

    // use all cores
    omp_set_num_threads(omp_get_max_threads());
    printf("ImageEvo running on %d thread(s)\n", omp_get_max_threads());
    printf("Press H to pause and S to save to file\n");

    int gen_ctr = 0;
    bool done = false, pause = false, take_screenshot = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_H) pause = !pause;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_S) take_screenshot = true;
        }

        if (take_screenshot) {
            take_screenshot = false;
            printf("Saved screenshot\n");
            stbi_write_png("screenshot.png", w, h, components, buffer.data(), 0);
        }

        if (!pause && gen_ctr < gen_limit) {
            NextGeneration(w, h, buffer, image.data);
            printf("Generation #%d\n", gen_ctr);
            gen_ctr++;
        }

        Render(renderer, texture, buffer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
