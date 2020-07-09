#include <GL/gl3w.h>
#include <SDL.h>

#include <vector>

#include <stb_image.h>

#include "renderer.h"

void DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
                     const void* userParam) {
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
                (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
        Panic("OpenGL Error: aborting");
    }
}

int main(int argc, char** argv) {
    // load the input image
    if (argc != 2) {
        printf("Usage: image_gpu <image path>\n");
        return 1;
    }
    printf("Loading image %s\n", argv[1]);
    int width, height, n;
    u8* image_data = stbi_load(argv[1], &width, &height, &n, 0);
    if (!image_data) {
        printf("Failed to load image data\n");
        return 1;
    };

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    // some linux WMs/Compositors will render artifacts and have other issues if this is not enabled
    // SDL_SetHintWithPriority(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0", SDL_HINT_OVERRIDE);

#if __APPLE__
    // GL 3.2 Core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.3
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("ImageEvo - GPU", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          width, height, window_flags);
    if (!window) {
        printf("Failed to create SDL_Window\n");
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    bool err = gl3wInit() != 0;
    if (err) {
        printf("Failed to initialize OpenGL loader\n");
        return 1;
    }

    // get some information about the hardware
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* model = glGetString(GL_RENDERER);
    printf("Detected hardware: %s %s\n", vendor, model);

    // debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(DebugCallback, nullptr);

    // framebuffer for next generation (off-screen)
    GLuint tex_handle;
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint scratch_buffer;
    glGenFramebuffers(1, &scratch_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, scratch_buffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_handle, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Failed to init off-screen framebuffer\n");
        return 1;
    }
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<u8> tex_data(width * height * 3);
    //glBindTexture(GL_TEXTURE_2D, tex_handle);
    //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data.data());
    //printf("First pixel: r=%u, g=%u, b=%u\n", tex_data[0], tex_data[1], tex_data[2]);
    //glBindTexture(GL_TEXTURE_2D, 0);

    Renderer renderer;
    renderer.Init();

    //std::array<Position, 3> test_pos = { Position(.5f, -.5f), Position(-.5f, -.5f), Position(.0f, .5f) };
    //std::array<Color, 3> test_color = { Color(1.f, 0.f, 0.f), Color(0.f, 1.f, 0.f), Color(0.f, 0.f, 1.f) };
    //renderer.PushTriangle(test_pos, test_color);
    //renderer.GenRandomTriangle();

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
            if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_G) {
                for (u32 i = 0; i < 100; i++) renderer.GenRandomTriangle();
                renderer.Draw(scratch_buffer);
            }
        }

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.Draw(0);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}