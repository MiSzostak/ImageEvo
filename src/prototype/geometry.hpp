#pragma once

#include <ctime>
#include <ostream>
#include <random>
#include <algorithm>

template <typename T>
struct Vec2 {
    union {
        struct {
            T x, y;
        };
    };
    Vec2() : x(0), y(0) {}
    Vec2(T x, T y) : x(x), y(y) {}
    inline Vec2<T> operator+(const Vec2<T>& rhs) const { return Vec2<T>(x + rhs.x, y + rhs.y); }
    inline Vec2<T> operator-(const Vec2<T>& rhs) const { return Vec2<T>(x - rhs.x, y - rhs.y); }
    inline Vec2<T> operator*(const float f) const { return Vec2<T>(x * f, y * f); }
    template <typename>
    friend std::ostream& operator<<(std::ostream& os, Vec2<T>& v);
};

template <typename T>
std::ostream& operator<<(std::ostream& os, Vec2<T>& v) {
    os << "(" << v.x << "," << v.y << ")";
    return os;
}

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec2u = Vec2<uint32_t>;

struct Ellipse {
    Vec2u origin;
    int major, minor;
    double angle;
    uint32_t color;
    Ellipse(Vec2u origin, int major, int minor, float angle, uint32_t color)
        : origin(origin), major(major), minor(minor), angle(angle), color(color){};
    void Mutate(int w, int h) {
        static std::default_random_engine e(std::time(nullptr));
        static std::uniform_real_distribution<> rand_angle(-5, 5);
        static std::uniform_int_distribution<> rand_origin(-10, 10);
        static std::uniform_int_distribution<> rand_axes(-10, 10);
        // static std::uniform_int_distribution<> rand_color(0, 0xFFFFFF);
        origin.x = std::clamp(int(origin.x) + rand_origin(e), 0, w);
        origin.y = std::clamp(int(origin.y) + rand_origin(e), 0, h);
        int d1 = rand_axes(e) + major, d2 = rand_axes(e) + minor;
        if (d1 >= 0) major = d1;
        if (d2 >= 0) minor = d2;
        angle = rand_angle(e);
        // color = rand_color(e);
    }
};

Ellipse RandomEllipse(int max_width, int max_height) {
    static std::default_random_engine e(std::time(nullptr));
    static std::uniform_real_distribution<> rand_angle(-5, 5);
    static std::uniform_int_distribution<> rand_origin(0, std::min(max_width, max_height));
    static std::uniform_int_distribution<> rand_axes(0, std::min(max_width, max_height) / 2);
    static std::uniform_int_distribution<> rand_color(0, 0xFFFFFF);
    return Ellipse(Vec2u(rand_origin(e), rand_origin(e)), rand_axes(e), rand_axes(e), rand_angle(e), rand_color(e));
}
