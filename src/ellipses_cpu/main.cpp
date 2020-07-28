#include <SDL.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <omp.h>

#include <cmath>
#include <cstdio>
#include <string>

#include "geometry.hpp"

void Render(SDL_Renderer* renderer, SDL_Texture* texture, cv::Mat& buffer) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);

    int pitch = 0;
    u8* displayPtr = nullptr;
    SDL_LockTexture(texture, nullptr, (void**)&displayPtr, &pitch);
    std::memcpy(displayPtr, buffer.data, buffer.total() * buffer.elemSize());
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void DrawEllipse(int width, int height, cv::Mat& buffer, Ellipse& e) {
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
                buffer.data[index] = e.color.r;
                buffer.data[index + 1] = e.color.g;
                buffer.data[index + 2] = e.color.b;
            }
        }
    }
}

void NextGeneration(int width, int height, cv::Mat& last_gen, const u8* original) {
    cv::Mat new_gen = last_gen.clone();
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
                    new_color.r = new_gen.data[index];
                    new_color.g = new_gen.data[index + 1];
                    new_color.b = new_gen.data[index + 2];
                }

                current_score += std::abs(int(original[index]) - int(new_color.r));
                current_score += std::abs(int(original[index + 1]) - int(new_color.g));
                current_score += std::abs(int(original[index + 2]) - int(new_color.b));

                last_score += std::abs(int(original[index]) - int(new_gen.data[index]));
                last_score += std::abs(int(original[index + 1]) - int(new_gen.data[index + 1]));
                last_score += std::abs(int(original[index + 2]) - int(new_gen.data[index + 2]));
            }
        }

        current_score /= (bb * bb * 3);
        last_score /= (bb * bb * 3);

        if (current_score < last_score) {
            std::memcpy(new_gen.data, last_gen.data, last_gen.total() * last_gen.elemSize());
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

    std::memcpy(last_gen.data, new_gen.data, new_gen.total() * new_gen.elemSize());
}

int ConvertVideo(std::string& video_path) {
    cv::VideoCapture capture(video_path, cv::CAP_GSTREAMER);
    if (!capture.isOpened()) {
        printf("Failed to open source video from %s\n", video_path.c_str());
        return 1;
    }
    // output should have the same properties
    printf("%f\n", capture.get(cv::CAP_PROP_FPS));
    cv::VideoWriter writer("video_out.avi",
                           cv::VideoWriter::fourcc('H', '2', '6', '4'), capture.get(cv::CAP_PROP_FPS),
                           cv::Size(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT)));

    // convert frame-by-frame
    u32 frame_counter = 0;
    while (1) {
        // get next frame
        cv::Mat frame;
        capture >> frame;
        // no frames left
        if (frame.empty()) break;

        // convert the frame
        // TODO

        // save frame
        writer << frame;
        frame_counter++;
    }

    printf("Finished converting video with %u frames\n", frame_counter);
    return 0;
}

int main(int argc, char** argv) {
    // use all cores
    omp_set_num_threads(omp_get_max_threads());
    printf("ImageEvo running on %d thread(s)\n", omp_get_max_threads());

    //printf("%s\n", cv::getBuildInformation().c_str());

    cv::CommandLineParser parser(argc, argv,
                                 "{@source|<none>|path to the input image}"
                                 "{ngenerations n|1000|number of generations}"
                                 "{headless h||no window, halt after ngenerations}"
                                 "{video v||use video as source and output}");

    std::string file_path = parser.get<std::string>("@source");
    int gen_limit = parser.get<int>("n");
    bool headless = parser.has("h");
    bool video = parser.has("v");

    if (file_path.empty()) {
        printf("No image specified\n");
        return 1;
    }

    if (gen_limit < 0) {
        printf("Invalid generation limit %d\n", gen_limit);
        return 1;
    }
    if (video && !headless) {
        printf("Video source can only be used in headless mode\n");
        return 1;
    }

    if (video) {
        int error = ConvertVideo(file_path);
        if (error) printf("Failed to convert video\n");
        return error;
    }

    cv::Mat buffer;
    cv::Mat image = cv::imread(file_path);
    //cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
    const int w = image.cols, h = image.rows, components = image.channels();

    if (!image.data || components != 3) {
        printf("Failed to load image %s\n", file_path.c_str());
        return 1;
    }

    // create a blurred background as the baseline
    cv::medianBlur(image, buffer, 151);

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

    int gen_ctr = 0;

    if (headless) {
        while (gen_ctr < gen_limit) {
            NextGeneration(w, h, buffer, image.data);
            gen_ctr++;
        }
        u64 start = file_path.find_last_of('/');
        std::string out_file = "out_" + file_path.substr(start == std::string::npos ? 0 : start);
        printf("Finished after %d generations, saving to %s\n", gen_ctr, out_file.c_str());
        cv::imwrite(out_file, buffer);
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

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (texture == nullptr) {
        printf("Failed to create texture\n");
        return 1;
    }

    printf("Press H to pause and S to save to file\n");
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
            cv::imwrite("screenshot.png", buffer);
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
