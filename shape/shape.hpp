#ifndef SHAPE
#define SHAPE

#include "../vec/vec.hpp"
#include <string>
#include <iostream>
#include <sstream>

struct Sphere {
    // (x-a^2) + (y-b^2) + (z-c^2) = r^2
    // CG:PaP 2nd ed. in C: p. 702
    Vec3 center; // a, b, c
    float radius; // r
    Vec3 color;
    float reflectiveness; // 0-1
    float emissiveness;
    Vec3 emissionColor;
};

std::string encodeSphere(Sphere *s);

Sphere decodeSphere(std::string in);

struct Triangle {
    Vec3 a, b, c;
    Vec3 color;
    float reflectiveness;
    float emissiveness;
    Vec3 emissionColor;
};

std::string encodeTriangle(Triangle *t);

Triangle decodeTriangle(std::string in);

std::string encodeColour(Vec3 c);

Vec3 decodeColour(std::stringstream *in);

#endif
