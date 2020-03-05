#pragma once

#include <ostream>

template <typename T>
struct Vec2 {
    union {
        struct {
            T x, y;
        };
        T raw[2];
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

struct Ellipse {
    Vec2i origin;
    int major, minor;
    float angle;
    Ellipse(Vec2i origin, int major, int minor, float angle)
        : origin(origin), major(major), minor(minor), angle(angle){};
};

