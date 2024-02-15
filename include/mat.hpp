#ifndef MAT
#define MAT

#include "vec.hpp"

struct Mat3 {
    float v[9];

    constexpr float& operator() (std::size_t i, std::size_t j) noexcept {
        return v[i*3 + j];
    }
    
    constexpr float const& operator() (std::size_t i, std::size_t j) const noexcept {
        return v[i*3 + j];
    }
};

constexpr
Vec3 operator*(Mat3 const& a, Vec3 const& b) noexcept {
    Vec3 res = {};
    res.x = (a(0, 0)*b.x) + (a(0, 1)*b.y) + (a(0, 2)*b.z);
    res.y = (a(1, 0)*b.x) + (a(1, 1)*b.y) + (a(1, 2)*b.z);
    res.z = (a(2, 0)*b.x) + (a(2, 1)*b.y) + (a(2, 2)*b.z);
    return res;
}

constexpr
Mat3 operator*(Mat3 const& a, Mat3 const& b) noexcept {
    Mat3 res = { {} };
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            res(i, j) = (a(i, 0)*b(0, j)) + (a(i, 1)*b(1, j)) + (a(i, 2)*b(2, j));
        }
    }
    return res;
}

struct Mat2 {
    float v[4];

    constexpr float& operator() (std::size_t i, std::size_t j) noexcept {
        return v[i*2 + j];
    }
    
    constexpr float const& operator() (std::size_t i, std::size_t j) const noexcept {
        return v[i*2 + j];
    }
};

constexpr
float det(Mat2 a) noexcept {
    return a(0,0)*a(1,1) - a(1, 0)*a(0, 1);
}

#endif
