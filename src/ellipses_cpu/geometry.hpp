#pragma once

#include <ctime>
#include <ostream>
#include <random>
#include <algorithm>
#include <cstdint>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

template <typename T>
struct Vec2 {
    T x, y;
    Vec2() : x(0), y(0) {}
    Vec2(T x, T y) : x(x), y(y) {}
};

struct Color {
    u8 r, g, b;
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec2u = Vec2<uint32_t>;

struct Ellipse {
    Vec2u origin;
    int major, minor;
    double angle;
    Color color;
    Ellipse(Vec2u origin, int major, int minor, float angle)
        : origin(origin), major(major), minor(minor), angle(angle), color(Color()){};
    void Mutate(int w, int h) {
        static std::default_random_engine e(std::time(nullptr));
        static std::uniform_real_distribution<> rand_angle(-5, 5);
        static std::uniform_int_distribution<> rand_origin(-10, 10);
        static std::uniform_int_distribution<> rand_axes(-10, 10);
        origin.x = std::clamp(int(origin.x) + rand_origin(e), 0, w);
        origin.y = std::clamp(int(origin.y) + rand_origin(e), 0, h);
        int d1 = rand_axes(e) + major, d2 = rand_axes(e) + minor;
        if (d1 >= 0) major = d1;
        if (d2 >= 0) minor = d2;
        angle = rand_angle(e);
    }
};

//template <typename T, T low, T high>
//static T NextRandom() {
//    static std::default_random_engine e(std::time(nullptr));
//    if constexpr (std::is_same<T, int>::value) {
//        static std::uniform_int_distribution<> rng_int(low, high);
//        return rng_int(e);
//    }
//    if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value) {
//        static std::uniform_real_distribution<> rng_real(low, high);
//        return rng_real(e);
//    }
//    return 0;
//}

Ellipse RandomEllipse(int max_width, int max_height) {
    static std::default_random_engine e(std::time(nullptr));
    static std::uniform_real_distribution<> rand_angle(-5, 5);
    static std::uniform_int_distribution<> rand_origin(0, std::min(max_width, max_height));
    static std::uniform_int_distribution<> rand_axes(0, std::min(max_width, max_height) / 2);
    return Ellipse(Vec2u(rand_origin(e), rand_origin(e)), rand_axes(e), rand_axes(e), rand_angle(e));
}
