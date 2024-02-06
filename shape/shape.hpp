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
    float specular; // 0-1
};

std::string encodeSphere(Sphere *s);

Sphere decodeSphere(std::string in);

struct Triangle {
    Vec3 a, b, c;
    Vec3 color;
    float specular;
};

struct PointLight {
    Vec3 center;
    Vec3 color;
    float brightness;
};

std::string encodePointLight(PointLight *p);

PointLight decodePointLight(std::string in);

std::string encodeTriangle(Triangle *t);

Triangle decodeTriangle(std::string in);

std::string encodeColour(Vec3 c);

Vec3 decodeColour(std::stringstream *in);

#endif
