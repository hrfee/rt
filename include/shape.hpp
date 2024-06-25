#ifndef SHAPE
#define SHAPE

#include "vec.hpp"
#include "tex.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

struct Sphere;
struct Triangle;
struct Container;
struct AAB;
struct Material;


struct Shape {
    Sphere *s;
    Triangle *t;
    Container *c;
    AAB *b;
    Material *m;
    bool debug; // Whether to be hidden in normal output or not.
};

struct Material {
    char *name;
    Vec3 color;
    int texId, normId, refId;
    bool noLighting;
    float opacity;
    float reflectiveness; // 0-1
    float specular; // 0-1
    float shininess;
    Material *next;
};

class MaterialStore {
    public:
        MaterialStore(): start(NULL), end(NULL), ptrs(NULL), names(NULL), count(0) {};
        Material *start, *end;
        void append(Material *m);
        void clear();
        void genLists();
        Material *byName(std::string name);
        Material **ptrs;
        char **names;
        int count;
    private:
        void clearLists();
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
    int id;
};

struct AAB {
    Vec3 min, max;
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
    Vec2 UVs[3]; // UV coordinates of a, b, c respectively
    bool plane; // Whether or not a plane defined by these points, or just a triangle
};

struct PointLight {
    Vec3 center;
    Vec3 color;
    float brightness;
    Vec3 specularColor;
    float specular;
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

struct Decoder {
    TexStore *tex, *norm, *ref;
    MaterialStore *mat;
    void setStores(TexStore *t = NULL, TexStore *n = NULL, TexStore *r = NULL, MaterialStore *m  = NULL) {
        usedMaterial = NULL;
        tex = t;
        norm = n;
        ref = r;
        mat = m;
    }
    Shape *decodeShape(std::string in);

    Material *decodeMaterial(std::string in, bool definition = false);

    std::string encodePointLight(PointLight *p);

    PointLight decodePointLight(std::string in);

    std::string encodeSphere(Shape *sh);

    Shape *decodeSphere(std::string in);

    std::string encodeTriangle(Shape *sh);

    Shape *decodeTriangle(std::string in);

    std::string encodeAAB(Shape *sh);

    Shape *decodeAAB(std::string in);
    
    void recalculateTriUVs(Shape *sh);
    
    Material *usedMaterial;
    void usingMaterial(std::string name);
    bool isUsingMaterial() { return usedMaterial != NULL; };
    void endUsingMaterial() { usedMaterial = NULL; };
};

std::string encodeColour(Vec3 c);

Vec3 decodeColour(std::stringstream *in);

Shape *emptyShape(bool noMaterial = false);
Material *emptyMaterial();

Bound *emptyBound();

Container *emptyContainer(bool plane = false);

int appendToContainer(Container *c, Bound *bo);
int appendToContainer(Container *c, Bound bo);
int appendToContainer(Container *c, Shape *sh);
int appendToContainer(Container *cParent, Container *c);

Bound *boundByIndex(Container *c, int i);

int clearContainer(Container *c, bool clearChildShapes = true);

CamPreset decodeCamPreset(std::string in);
std::string encodeCamPreset(CamPreset *p);


#endif
