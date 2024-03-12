#ifndef SHAPE
#define SHAPE

#include "vec.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

struct Sphere;
struct Triangle;
struct Container;

struct Shape {
    Sphere *s;
    Triangle *t;
    Container *c;
    Vec3 color;
    float opacity;
    float reflectiveness; // 0-1
    float specular; // 1-inf
    float shininess;
    bool debug; // Whether to be hidden in normal output or not.
};

struct Bound {
    Vec3 min, max, centroid; // Bounding box
    Shape *s;
    Bound *next;
};

// When plane is true, acts as a very simple optimization.
// Defined in .map file as:
// container a <vec3> b <vec3> c <vec3> d <vec3> {
// <shapes>
// }
// Note: abcd must be arranged clockwise!
// When plane is false, acts as an Axis-Aligned Bounding Box (AABB),
// Where a and b are the corners.
struct Container {
    Vec3 a, b, c, d;
    Bound *start;
    Bound *end;
    bool plane;
    int splitAxis;
    bool voxelSubdiv;
    int size;
};

struct Sphere {
    // (x-a^2) + (y-b^2) + (z-c^2) = r^2
    // CG:PaP 2nd ed. in C: p. 702
    Vec3 center; // a, b, c
    float radius; // r
    float thickness; // fraction of radius. 0 == hollow, 1 == filled.
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

struct CamPreset {
    std::string name;
    float phi, theta;
    Vec3 pos;
    float fov;
    CamPreset() {
        name.clear();
        phi = 0.f;
        theta = 0.f;
        pos = {0,0,0};
        fov = 0.f;
    };
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

Bound *emptyBound();

Container *emptyContainer(bool plane = false);

void appendToContainer(Container *c, Bound *bo);

void appendToContainer(Container *c, Bound bo);

void appendToContainer(Container *c, Shape *sh);

void appendToContainer(Container *cParent, Container *c);

Bound *boundByIndex(Container *c, int i);

int clearContainer(Container *c, bool clearChildShapes = true);

CamPreset decodeCamPreset(std::string in);
std::string encodeCamPreset(CamPreset *p);

#endif
