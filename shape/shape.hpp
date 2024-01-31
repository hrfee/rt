#ifndef SHAPE
#define SHAPE

#include "../vec/vec.hpp"
#include <string>

struct Sphere {
    // (x-a^2) + (y-b^2) + (z-c^2) = r^2
    // CG:PaP 2nd ed. in C: p. 702
    Vec3 center; // a, b, c
    float radius; // r
    Vec3 color;
    float reflectiveness; // 0-1
};

std::string encodeSphere(Sphere *s);

Sphere decodeSphere(std::string in);

struct Triangle {
    Vec3 a, b, c;
    Vec3 color;
    float reflectiveness;
};

std::string encodeTriangle(Triangle *t);

Triangle decodeTriangle(std::string in);

#endif
