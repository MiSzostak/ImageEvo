#include "renderer.h"

#include <random>
#include <ctime>
#include <algorithm>

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
    glVertexAttribPointer(pos_index, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    colors.Init();
    GLuint clr_index = GetAttributeIndex("vertex_color");
    glEnableVertexAttribArray(clr_index);
    glVertexAttribPointer(clr_index, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void Renderer::GenRandomTriangle(GLuint tex_handle) {
    static std::default_random_engine e(std::time(nullptr));
    static std::uniform_real_distribution<> rng_pos(-1.f, 1.f);
    static std::uniform_real_distribution<> rng_rgb(0, 1);

    if (curr_vertices + 3 > BUFFER_SIZE) Panic("Vertex buffer full");

    // save the current frame data
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, prev_gen_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    float mono_color = rng_rgb(e);
    for (u32 i=0; i<3; i++) {
        positions.Set(curr_vertices, Position(rng_pos(e), rng_pos(e)));
        colors.Set(curr_vertices, Color(mono_color, mono_color, mono_color));
        curr_vertices++;
    }
}

void Renderer::CheckFitness(GLuint tex_handle) {
    // we call this after GenTriangle and Draw
    // the data from the previous gen has been saved at the start of GenTriangle

    // load new polygon
    // TODO: use u32 for position and convert in shader

    auto& v1 = positions.ptr[curr_vertices - 1];
    auto& v2 = positions.ptr[curr_vertices - 2];
    auto& v3 = positions.ptr[curr_vertices - 3];
    //auto& c1 = colors.ptr[curr_vertices - 1], c2 = colors.ptr[curr_vertices - 2], c3 = colors.ptr[curr_vertices - 3];

    // bounding box
    u32 minX = (u32) ((std::min({v1.x, v2.x, v3.x}) + 1) * (width / 2));
    u32 maxX = (u32) ((std::max({v1.x, v2.x, v3.x}) + 1) * (width / 2));
    u32 minY = (u32) ((std::min({v1.y, v2.y, v3.y}) + 1) * (height / 2));
    u32 maxY = (u32) ((std::max({v1.y, v2.y, v3.y}) + 1) * (height / 2));
    //printf("BB: (%u,%u) - (%u,%u)\n", minX, minY, maxX, maxY);

    // get data from off-screen framebuffer (this will probably be the bottleneck later on)
    glBindTexture(GL_TEXTURE_2D, tex_handle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, new_gen_data.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    double new_score = 0.0, prev_score = 0.0;
    for (u32 y = minY; y < maxY; y++) {
        for (u32 x = minX; x < maxX; x++) {
            u32 base_index = x + y * width;
            for (u32 i = 0; i < 3; i++) {
                new_score += std::abs(source_image[base_index + i] - new_gen_data[base_index + i]);
                prev_score += std::abs(source_image[base_index + i] - prev_gen_data[base_index + i]);
            }
        }
    }

    u32 length_x = maxX - minX;
    u32 length_y = maxY - minY;
    new_score /= (length_x * length_y * 3);
    prev_score /= (length_x * length_y * 3);

    static u32 hits = 0;
    if (new_score < prev_score) {
        printf("Hit #%u\n", hits++);
    } else {
        if (curr_vertices >= 3) curr_vertices -= 3;
        printf("Miss :(\n");
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

void Renderer::Draw(GLuint fb_handler) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb_handler);

    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)curr_vertices);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    for (;;) {
        auto result = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //curr_vertices = 0;
}

GLuint Renderer::GetAttributeIndex(const char* attribute) {
    GLint index = glGetAttribLocation(program, attribute);
    Assert(index >= 0);

    return static_cast<GLuint>(index);
}
