#ifndef SHAPE
#define SHAPE

#include "vec.hpp"
#include <string>
#include <iostream>
#include <sstream>

struct Sphere;
struct Triangle;
struct ContainerQuad;

struct Shape {
    Sphere *s;
    Triangle *t;
    ContainerQuad *c;
    Vec3 color;
    float opacity;
    float reflectiveness; // 0-1
    float specular; // 1-inf
    float shininess;
    Shape *next;
};

// Very simple optimization. Defined in .map file as:
// container a <vec3> b <vec3> c <vec3> d <vec3> {
// <shapes>
// }
// Note: abcd must be arranged clockwise!
struct ContainerQuad {
    Vec3 a, b, c, d;
    Shape *start;
    Shape *end;
};

struct Sphere {
    // (x-a^2) + (y-b^2) + (z-c^2) = r^2
    // CG:PaP 2nd ed. in C: p. 702
    Vec3 center; // a, b, c
    float radius; // r
};

struct Triangle {
    Vec3 a, b, c;
};

struct PointLight {
    Vec3 center;
    Vec3 color;
    float brightness;
    float specular;
    Vec3 specularColor;
};

std::string encodePointLight(PointLight *p);

PointLight decodePointLight(std::string in);

std::string encodeSphere(Shape *sh);

Shape *decodeSphere(std::string in);

std::string encodeTriangle(Shape *sh);

Shape *decodeTriangle(std::string in);

std::string encodeColour(Vec3 c);

Vec3 decodeColour(std::stringstream *in);

Shape *emptyShape();

#endif
