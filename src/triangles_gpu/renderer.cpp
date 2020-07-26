#include "renderer.h"

#include <random>
#include <ctime>
#include <algorithm>

#include <stb_image_write.h>

#include "common.h"
#include "shader.h"

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(program);
}

void Renderer::Init(int w, int h, u8* data) {
    width = w;
    height = h;
    source_image = data;
    new_gen_data.resize(width * height * 3);
    prev_gen_data.resize(width * height * 3);
    // compile shaders
    const std::string path = "../../../src/image_gpu/shader";
    Shader shader(path + ".vs", path + ".fs");
    program = shader.program_id;

    shader.Use();

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    positions.Init();
    GLuint pos_index = GetAttributeIndex("vertex_position");
    glEnableVertexAttribArray(pos_index);
    glVertexAttribIPointer(pos_index, 2, GL_INT, 0, nullptr);

    colors.Init();
    GLuint clr_index = GetAttributeIndex("vertex_color");
    glEnableVertexAttribArray(clr_index);
    glVertexAttribIPointer(clr_index, 3, GL_UNSIGNED_BYTE, 0, nullptr);

    shader.SetInt("width", width);
    shader.SetInt("height", height);
}

void Renderer::SetOffscreenFB(GLuint fb_handle, GLuint texture_handle) {
    offscreen_fb = fb_handle;
    offscreen_texture = texture_handle;
}

void Renderer::NextGeneration() {
    static u32 gen_count = 0;
    printf("Generation %u\n", gen_count);

    CalcBaseline();
    GenRandomTriangle();
    Draw(offscreen_fb);
    CheckFitness();

    u32 mutation_count = 0u;
    bool found_better_fit = false;
    while (mutation_count < 100u) {
        MutateTriangle();
        Draw(offscreen_fb);
        if (CheckFitness()) found_better_fit = true;

        //if (found_better_fit) mutation_count++;
        mutation_count++;
    }

    // load best fit
    LoadTriangle();
    gen_count++;
}

void Renderer::MutateTriangle() {
    static std::default_random_engine e(std::time(nullptr));
    static std::uniform_int_distribution<> rng_diff(-50, 50);

    if (curr_vertices < 3) Panic("No triangle available");

    for (u32 i = curr_vertices - 3; i < curr_vertices; i++) {
        positions.ptr[i].x = std::clamp<s32>(positions.ptr[i].x + rng_diff(e), 0, width);
        positions.ptr[i].y = std::clamp<s32>(positions.ptr[i].y + rng_diff(e), 0, height);
    }
}

void Renderer::GenRandomTriangle() {
    static std::default_random_engine e(std::time(nullptr));
    static std::uniform_int_distribution<> rng_pos_x(0, width - 1);
    static std::uniform_int_distribution<> rng_pos_y(0, height - 1);

    if (curr_vertices + 3 > BUFFER_SIZE) Panic("Vertex buffer full");

    // save the current frame data
    //glBindTexture(GL_TEXTURE_2D, offscreen_texture);
    //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, prev_gen_data.data());
    //glBindTexture(GL_TEXTURE_2D, 0);

    s32 center_x = 0, center_y = 0;

    for (u32 i=0; i<3; i++) {
        s32 x = rng_pos_x(e), y = rng_pos_y(e);
        //printf("New pos (%d, %d)\n", x, y);
        center_x += x, center_y += y;
        //Assert((x >= 0 && x < width) && (y >= 0 && y < height));
        positions.Set(curr_vertices, Position(x, y));
        curr_vertices++;
    }

    center_x /= 3, center_y /= 3;
    for (u32 i=curr_vertices-3; i<curr_vertices; i++) {
        u8* source_pixel = &source_image[(center_x + width * center_y) * 3];
        colors.Set(i, Color(source_pixel[0], source_pixel[1], source_pixel[2]));
    }
}

bool Renderer::CheckFitness() {
    // we call this after GenTriangle and Draw
    // the data from the previous gen has been saved at the start of GenTriangle

    auto& v1 = positions.ptr[curr_vertices - 3];
    auto& v2 = positions.ptr[curr_vertices - 2];
    auto& v3 = positions.ptr[curr_vertices - 1];
    //auto& c1 = colors.ptr[curr_vertices - 1], c2 = colors.ptr[curr_vertices - 2], c3 = colors.ptr[curr_vertices - 3];

    // bounding box
    s32 minX = std::min({v1.x, v2.x, v3.x});
    s32 maxX = std::max({v1.x, v2.x, v3.x});
    s32 minY = std::min({v1.y, v2.y, v3.y});
    s32 maxY = std::max({v1.y, v2.y, v3.y});
    //printf("BB: (%u,%u) - (%u,%u)\n", minX, minY, maxX, maxY);

    // get data from off-screen framebuffer (this will probably be the bottleneck later on)
    glBindTexture(GL_TEXTURE_2D, offscreen_texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, new_gen_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    //stbi_write_png("after_fit_check.png", width, height, 3, new_gen_data.data(), 0);
    //stbi_write_png("pic.png", width, height, 3, source_image, 0);

    double new_score = 0.0, old_score = 0.0;
    for (s32 y = minY; y < maxY; y++) {
        for (s32 x = minX; x < maxX; x++) {
            s32 base_index = (x + y * width) * 3;
            for (s32 i = 0; i < 3; i++) {
                new_score += std::abs(int(source_image[base_index + i]) - int(new_gen_data[base_index + i]));
                old_score += std::abs(int(source_image[base_index + i]) - int(prev_gen_data[base_index + i]));
            }
        }
    }

    s32 length_x = (maxX - minX);
    s32 length_y = (maxY - minY);
    new_score /= (length_x * length_y * 3);
    old_score /= (length_x * length_y * 3);

    if (new_score < old_score) {
        if (new_score < best_score) {
            best_score = new_score;
            StoreTriangle();
        }
        return true;
    } else {
        // restore best fit
        //LoadTriangle();
        return false;
    }
}

void Renderer::CalcBaseline() {
    // save the current frame data
    glBindTexture(GL_TEXTURE_2D, offscreen_texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, prev_gen_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_write_png("new_gen.png", width, height, 3, prev_gen_data.data(), 0);

    // calculate the score to beat
    best_score = 0.0;
    for (s32 y = 0; y < height; y++) {
        for (s32 x = 0; x < width; x++) {
            s32 base_index = (x + y * width) * 3;
            for (s32 i = 0; i < 3; i++) {
                best_score += std::abs(int(source_image[base_index + i]) - int(prev_gen_data[base_index + i]));
            }
        }
    }

    best_score /= (width * height * 3);
}

void Renderer::StoreTriangle() {
    u32 start = curr_vertices - 3;
    for (u32 i = 0; i < 3; i++) {
        best_triangle_pos[i] = positions.ptr[start + i];
        best_triangle_color[i] = colors.ptr[start + i];
    }
}

void Renderer::LoadTriangle() {
    u32 start = curr_vertices - 3;
    for (u32 i = 0; i < 3; i++) {
        positions.ptr[start + i] = best_triangle_pos[i];
        colors.ptr[start + i] = best_triangle_color[i];
    }
}

void Renderer::PushTriangle(std::array<Position, 3> vertices, std::array<Color, 3> clrs) {
    if (curr_vertices + 3 > BUFFER_SIZE) Panic("Vertex buffer full");

    for (u32 i=0; i<3; i++) {
        positions.Set(curr_vertices, vertices[i]);
        colors.Set(curr_vertices, clrs[i]);
        curr_vertices++;
    }
}

void Renderer::Draw(GLuint fb_handle) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb_handle);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    //positions.Clear();
    //colors.Clear();
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)curr_vertices);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    for (;;) {
        auto result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Renderer::GetAttributeIndex(const char* attribute) {
    GLint index = glGetAttribLocation(program, attribute);
    Assert(index >= 0);

    return static_cast<GLuint>(index);
}
