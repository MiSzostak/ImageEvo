#pragma once

#include <GL/gl3w.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "common.h"

class Shader {
public:
    GLuint program_id;

    Shader(const std::string& vertex_path, const std::string& fragment_path) {
        std::string vertex_code, fragment_code;
        std::ifstream v_shader_file, f_shader_file;
        v_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        f_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            v_shader_file.open(vertex_path);
            f_shader_file.open(fragment_path);

            std::stringstream v_shader_stream, f_shader_stream;
            v_shader_stream << v_shader_file.rdbuf();
            f_shader_stream << f_shader_file.rdbuf();

            vertex_code = v_shader_stream.str();
            fragment_code = f_shader_stream.str();
        } catch (std::ifstream::failure& e) {
            Panic("Failed to load shader files");
        }
        const char* v_shader_code = vertex_code.c_str();
        const char* f_shader_code = fragment_code.c_str();

        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &v_shader_code, nullptr);
        glCompileShader(vertex);
        // fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &f_shader_code, nullptr);
        glCompileShader(fragment);

        program_id = glCreateProgram();
        glAttachShader(program_id, vertex);
        glAttachShader(program_id, fragment);
        glLinkProgram(program_id);

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void Use() { glUseProgram(program_id); }

    void SetBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(program_id, name.c_str()), (int)value);
    }
    void SetInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(program_id, name.c_str()), value);
    }
    void SetFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(program_id, name.c_str()), value);
    }
};
