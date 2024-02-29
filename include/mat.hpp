#ifndef MAT
#define MAT

#include "vec.hpp"


struct Mat4 {
    float v[16];

    constexpr float& operator() (std::size_t i, std::size_t j) noexcept {
        return v[i*4 + j];
    }
    
    constexpr float const& operator() (std::size_t i, std::size_t j) const noexcept {
        return v[i*4 + j];
    }
};

inline constexpr Mat4 mat44Identity = {{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};

constexpr
Vec3 operator*(Mat4 const& a, Vec3 const& b) noexcept {
    float res[3] = {0, 0, 0};

	for (int i = 0; i < 3; i++) {
		res[i] = (a(i, 0) * b.x) +
			(a(i, 1) * b.y) +
			(a(i, 2) * b.z) +
			(a(i, 3) * 1.f); // We have no 4th component, so assume it's 1
	}
	return Vec3{res[0], res[1], res[2]};
}

constexpr
Vec3 operator*(Vec3 const& a, Mat4 const& b) noexcept {
    return b * a;
}

inline
Mat4 rotateX(float rad) noexcept {
	return Mat4{{
        1, 0, 0, 0,
        0, std::cos(rad), -std::sin(rad), 0,
        0, std::sin(rad), std::cos(rad), 0,
        0, 0, 0, 1
    }};
}

inline
Mat4 rotateY(float rad) noexcept {
	return Mat4{{
        std::cos(rad), 0, std::sin(rad), 0,
        0, 1, 0, 0,
        -std::sin(rad), 0, std::cos(rad), 0,
        0, 0, 0, 1
    }};
}

inline
Mat4 rotateZ(float rad) noexcept {
	return Mat4{{
        std::cos(rad), -std::sin(rad), 0, 0,
        std::sin(rad), std::cos(rad), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }};
}

inline
Mat4 scale(float s) noexcept {
	return Mat4{{
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, s, 0,
        0, 0, 0, 1
	}};
}

inline
Mat4 translateMat(Vec3 a) noexcept {
    return Mat4{{
        1, 0, 0, a.x,
        0, 1, 0, a.y,
        0, 0, 1, a.z,
        0, 0, 0, 1
    }};
}

constexpr
Mat4 operator*(Mat4 const& a, Mat4 const& b) noexcept {
    Mat4 res = { {} };
    for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			res(i, j) = (a(i, 0) * b(0, j)) + (a(i, 1) * b(1, j)) + (a(i, 2) * b(2, j)) + (a(i, 3) * b(3, j));
		}
	}
    return res;
}

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
