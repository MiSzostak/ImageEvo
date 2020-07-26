#version 330 core

uniform int width;
uniform int height;

in ivec2 vertex_position;
in uvec3 vertex_color;

out vec3 color;

void main() {
    // convert from 0;width to -1;1
    float xpos = (float(vertex_position.x) / (width / 2)) - 1.0;
    // convert from 0;height to -1;1
    float ypos = (float(vertex_position.y) / (height / 2)) - 1.0;

    gl_Position.xyzw = vec4(xpos, ypos, 0.0, 1.0);

    // convert from 0;256 to 0;1
    color = vec3(float(vertex_color.r) / 255, float(vertex_color.g) / 255, float(vertex_color.b) / 255);
}
