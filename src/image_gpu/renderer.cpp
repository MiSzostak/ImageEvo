#include "renderer.h"

#include <random>
#include <ctime>

#include "common.h"
#include "shader.h"

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(program);
}

void Renderer::Init() {
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

void Renderer::GenRandomTriangle() {
    static std::default_random_engine e(std::time(nullptr));
    static std::uniform_real_distribution<> rng_pos(-1.f, 1.f);
    static std::uniform_real_distribution<> rng_rgb(0, 1);

    if (curr_vertices + 3 > BUFFER_SIZE) Panic("Vertex buffer full");

    for (u32 i=0; i<3; i++) {
        positions.Set(curr_vertices, Position(rng_pos(e), rng_pos(e)));
        colors.Set(curr_vertices, Color(rng_rgb(e), rng_rgb(e), rng_rgb(e)));
        curr_vertices++;
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
