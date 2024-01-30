#ifndef VEC
#define VEC

#include <cstdio>
#include <cstdint>

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
Vec3 operator/(Vec3 a, float b) noexcept {
    return Vec3{ a.x/b, a.y/b, a.z/b };
}

constexpr
float dot(Vec3 a, Vec3 b) noexcept {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

struct Vec3ui {
    uint8_t x, y, z;
};

struct Vec3c {
    char x, y, z;
};

#endif
