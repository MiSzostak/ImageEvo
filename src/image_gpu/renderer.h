#pragma once

#include <GL/gl3w.h>

#include <array>
#include <vector>
#include <cstring>
#include <limits>

#include "common.h"

struct Position {
    Position() {};
    Position(GLint x, GLint y) : x(x), y(y) {};
    GLint x, y;
};

struct Color {
    Color() {};
    Color(GLubyte r, GLubyte g, GLubyte b) : r(r), g(g), b(b) {};
    GLubyte r, g, b;
};

class Renderer {
public:
    ~Renderer();
    void Init(int w, int h, u8* data);
    void SetOffscreenFB(GLuint fb_handle, GLuint texture_handle);

    void Draw(GLuint fb_handle);
    void PushTriangle(std::array<Position, 3> vertices, std::array<Color, 3> color);
    void NextGeneration();
    void GenRandomTriangle();
    bool CheckFitness();
    void StoreTriangle();
    void LoadTriangle();
    void CalcBaseline();
private:
    void MutateTriangle();
    GLuint GetAttributeIndex(const char* attribute);

    static constexpr u32 BUFFER_SIZE = 2048 * 1024;

    template <typename T>
    struct Buffer {
        void Init() {
            glGenBuffers(1, &handle);
            glBindBuffer(GL_ARRAY_BUFFER, handle);

            constexpr GLsizeiptr size = sizeof(T) * BUFFER_SIZE;
            glBufferStorage(GL_ARRAY_BUFFER, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
            ptr = static_cast<T*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT));
            Assert(ptr);

            std::memset((void*)ptr, 0, size);
        }

        ~Buffer() {
            glBindBuffer(GL_ARRAY_BUFFER, handle);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glDeleteBuffers(1, &handle);
        }

        void Set(u32 index, T value) {
            Assert(index < BUFFER_SIZE);
            ptr[index] = value;
        }

        void Clear() {
            constexpr u32 size = sizeof(T) * BUFFER_SIZE;
            std::memset(ptr, 0, size);
        }

        GLuint handle = 0;
        T* ptr = nullptr;
    };

    int width = 0, height = 0;
    u8* source_image = nullptr;
    std::vector<u8> new_gen_data, prev_gen_data;
    GLuint offscreen_fb = 0, offscreen_texture = 0;

    Buffer<Position> positions;
    Buffer<Color> colors;
    u32 curr_vertices = 0;

    double best_score = std::numeric_limits<double>().max();
    Position best_triangle_pos[3];
    Color best_triangle_color[3];

    GLuint program = 0;
    GLuint VAO = 0;
};
