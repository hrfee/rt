#ifndef VEC
#define VEC

#include <cmath>
#include <cstdio>
#include <cstdint>

struct Vec2 {
    float x, y;
};

constexpr
Vec2 operator+(Vec2 a, Vec2 b) noexcept {
    return Vec2{ a.x+b.x, a.y+b.y };
}

constexpr
Vec2 operator-(Vec2 a, Vec2 b) noexcept {
    return Vec2{ a.x-b.x, a.y-b.y };
}

constexpr
Vec2 operator*(Vec2 a, float b) noexcept {
    return Vec2{ a.x*b, a.y*b };
}
constexpr
Vec2 operator*(float a, Vec2 b) noexcept {
    return b * a;
}

constexpr
Vec2 operator/(Vec2 a, float b) noexcept {
    return Vec2{ a.x/b, a.y/b };
}

struct Vec3 {
    float x, y, z;
};

constexpr
Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return Vec3{ a.x+b.x, a.y+b.y, a.z+b.z };
}

constexpr
Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return Vec3{ a.x-b.x, a.y-b.y, a.z-b.z };
}

constexpr
Vec3 operator*(Vec3 a, float b) noexcept {
    return Vec3{ a.x*b, a.y*b, a.z*b };
}
constexpr
Vec3 operator*(float a, Vec3 b) noexcept {
    return b * a;
}

constexpr
Vec3 operator*(Vec3 a, Vec3 b) noexcept {
    return Vec3{a.x*b.x, a.y*b.y, a.z*b.z};
}

constexpr
Vec3 operator/(Vec3 a, float b) noexcept {
    return Vec3{ a.x/b, a.y/b, a.z/b };
}

constexpr
bool operator==(Vec3 a, Vec3 b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

constexpr
bool operator!=(Vec3 a, Vec3 b) noexcept { return !(a == b); }

constexpr
float dot(Vec3 a, Vec3 b) noexcept {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

constexpr
Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return Vec3{
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}

constexpr
float mag(Vec3 a) noexcept {
    return std::sqrt(dot(a, a));
}

constexpr
Vec3 norm(Vec3 a) noexcept {
    return a/mag(a);
}

struct Vec3ui {
    uint8_t x, y, z;
};

struct Vec3c {
    char x, y, z;
};

#endif
