#ifndef SHAPE
#define SHAPE

#include "mat.hpp"
#include "vec.hpp"
#include "tex.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>

class Sphere;
class Triangle;
class Container;
class AAB;
struct Material;
class Shape;
struct Bound;

namespace ShapeType {
    const int Sphere = 0;
    const int Triangle = 1;
    const int AAB = 2;
}

inline const float EPSILON = 0.00001f;
inline const float RI_AIR = 1.f;
inline const float RI_GLASS = 1.52f;


struct Transform {
    Vec3 translate;
    Vec3 rotate;
    float scale;
    void reset() {
        translate = {0,0,0};
        rotate = {0,0,0};
        scale = 1.f;
    }
    bool needed() {
        return !(translate.x == 0.f && translate.y == 0.f && translate.z == 0.f && rotate.x == 0.f && rotate.y == 0.f && rotate.z == 0.f && scale == 1.f);
    }
    Mat4 build() {
        Mat4 trans = mat44Identity;
        trans = trans * translateMat(translate);
        if (rotate.x != 0.f) trans = trans * rotateX(rotate.x);
        if (rotate.y != 0.f) trans = trans * rotateY(rotate.y);
        if (rotate.z != 0.f) trans = trans * rotateZ(rotate.z);
        if (scale != 1.f) trans = trans * transformScale(scale);
        return trans;
    }
};

struct Bound {
    Vec3 min, max, centroid; // Bounding box
    Shape *s;
    Bound *next;
    void grow(Bound *src);
};

class Shape {
    public:
        Material *material;
        Transform transform;
        bool transformDirty;
        bool debug;
        Shape() {
            material = NULL;
            debug = false;
            transformDirty = true;
            transform.reset();
        };
        virtual Shape *clone() = 0;
        Shape(Shape *sh);
        virtual Material *mat() { return material; };
        virtual Transform& trans() { return transform; };
        virtual void bounds(Bound *bo) = 0;
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL) = 0;
        virtual bool intersects(Vec3 p0, Vec3 delta) = 0;
        virtual void applyTransform() = 0;
        virtual void bakeTransform() = 0;
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1) = 0;
        virtual bool envelops(Vec3 mn, Vec3 mx) { return false; };
        virtual bool envelops(Bound& bo) { return envelops(bo.min, bo.max); };
        virtual int clear(bool deleteShapes = true) { return 0; };
        virtual std::string name() = 0;
        virtual int type() = 0;
        virtual ~Shape() {};
    // Note no destructor for the dynamically allocated "material",
    // This is because the material is allocated and given by a MaterialStore,
    // which itself handles destruction in its clear() method.
};

/* struct Shape {
    Sphere *s;
    Triangle *t;
    Container *c;
    AAB *b;
    Material *m;
    Transform trans;
    bool debug; // Whether to be hidden in normal output or not.
}; */

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
    bool hasTexture() { 
        return texId != -1 || normId != -1 || refId != -1;
    };
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

// Bound with large min and low max to be used when find bounds of objects.
inline constexpr Bound growingBound = {{1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}, {0, 0, 0}, NULL, NULL};

// When plane is true, acts as a very simple optimization.
// Defined in .map file as:
// container a <vec3> b <vec3> c <vec3> d <vec3> {
// <shapes>
// }
// Note: abcd must be arranged clockwise!
// When plane is false, acts as an Axis-Aligned Bounding Box (AABB),
// Where a and b are the corners.
/* struct Container {
    Vec3 a, b, c, d;
    Bound *start;
    Bound *end;
    bool plane;
    int splitAxis;
    bool voxelSubdiv;
    int size;
    int id;
}; */

class AAB: public Shape {
    public:
        Vec3 min, max, oMin, oMax;
        AAB() {
            min = {0, 0, 0};
            max = {0, 0, 0};
            oMin = min;
            oMax = max;
        };
        AAB(Shape *sh) {
            AAB *b = static_cast<AAB*>(sh);
            material = b->material;
            transform = b->transform;
            min = b->min;
            max = b->max;
            oMin = b->oMin;
            oMax = b->oMax;
        };
        virtual Shape *clone() {
            return new AAB(this);
        };

        Vec2 getUV(Vec3 hit, Vec3 normal);
        AAB(Vec3 mn, Vec3 mx): min(mn), max(mx) {};
        virtual std::string name() { return std::string("Box"); };
        virtual int type() { return ShapeType::AAB; };
        virtual void bounds(Bound *bo);
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        virtual bool envelops(Vec3 mn, Vec3 mx);
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
};
// See note in shape.cpp for why this is here.
float meetAABB(Vec3 p0, Vec3 delta, Vec3 a, Vec3 b);

class Container: public AAB {
    public:
        Bound *start;
        Bound *end;
        bool plane;
        int splitAxis;
        bool voxelSubdiv;
        int size;
        int id;
        Container(bool isPlane = false) {
            plane = isPlane;
            voxelSubdiv = 0;
            start = NULL;
            end = NULL;
            size = 0;
            splitAxis = -1;
            id = 0;
        };
        Container(Shape *sh) {
            Container *c = static_cast<Container*>(sh);
            material = c->material;
            transform = c->transform;
            min = c->min;
            max = c->max;
            oMin = c->oMin;
            oMax = c->oMax;
            plane = c->plane;
            voxelSubdiv = c->voxelSubdiv;
            start = c->start;
            end = c->end;
            size = c->size;
            splitAxis = c->splitAxis;
            id = c->id;
        };
        virtual Shape *clone() {
            return new Container(this);
        };
        virtual std::string name() { return std::string("Container"); };
        int append(Bound *bo);
        int append(Bound bo);
        int append(Shape *sh);
        int append(Container *c);
       
        Bound *at(int i);

        int clear(bool deleteShapes = true);
};

class Sphere: public Shape {
    public:
        Vec3 center, oCenter;
        float radius, oRadius;
        float thickness;
        Vec2 getUV(Vec3 hit);
        Sphere() {
            center = {0.f, 0.f, 0.f};
            oCenter = center;
            radius = 0.f;
            oRadius = radius;
            thickness = 1.f;
        }
        Sphere(Shape *sh) {
            Sphere *s = static_cast<Sphere*>(sh);
            material = s->material;
            transform = s->transform;
            center = s->center;
            oCenter = s->oCenter;
            radius = s->radius;
            oRadius = s->oRadius;
            thickness = s->thickness;
        };
        virtual Shape *clone() {
            return new Sphere(this);
        };
        virtual std::string name() { return std::string("Sphere"); };
        virtual int type() { return ShapeType::Sphere; };
        virtual void bounds(Bound *bo);
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        virtual bool envelops(Vec3 mn, Vec3 mx);

        void refractSolid(float r0, float r1, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
};

/* struct Sphere {
    // (x-a^2) + (y-b^2) + (z-c^2) = r^2
    // CG:PaP 2nd ed. in C: p. 702
    Vec3 center; // a, b, c
    float radius; // r
    float thickness; // fraction of radius. 0 == hollow, 1 == filled.
}; */

class Triangle: public Shape {
    public: 
        Vec3 a, b, c, oA, oB, oC;
        Vec2 UVs[3]; // UV coordinates of a, b, c respectively
        bool plane; // Whether or not a plane defined by these points, or just a triangle
        Triangle() {
            a = {0, 0, 0}, b = a, c = a;
            oA = a, oB = b, oC = c;
            UVs[0] = {0,0};
            UVs[1] = {0,0};
            UVs[2] = {0,0};
            plane = false;
        };
        Triangle(Shape *sh) {
            Triangle *t = static_cast<Triangle*>(sh);
            material = t->material;
            transform = t->transform;
            a = t->a;
            b = t->b;
            c = t->c;
            oA = t->oA;
            oB = t->oB;
            oC = t->oC;
            std::memcpy(UVs, t->UVs, 3*sizeof(Vec2));
            plane = t->plane;
        };
        virtual Shape *clone() {
            return new Triangle(this);
        };
        virtual std::string name() { return plane ? std::string("Plane") : std::string("Triangle"); };
        virtual int type() { return ShapeType::Triangle; };
        Vec3 visibleNormal(Vec3 delta);
        float intersectsPlane(Vec3 p0, Vec3 delta, Vec3 normal);
        bool projectAndPiP(Vec3 normal, Vec3 hit);
        bool PiP(Vec2 projHit, Vec2 projA, Vec2 projB, Vec2 projC);
        float intersectMT(Vec3 p0, Vec3 delta, Vec3 *bary);
        
        virtual void bounds(Bound *bo);
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        // A triangle can't envelop a 3d object, so envelops() is left empty (returning false).
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
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
    void decodeShape(Shape *sh, std::string in);

    Material *decodeMaterial(std::string in, bool definition = false);

    // std::string encodePointLight(PointLight *p);

    PointLight decodePointLight(std::string in);

    // std::string encodeSphere(Shape *sh);

    Sphere *decodeSphere(std::string in);

    // std::string encodeTriangle(Shape *sh);

    Triangle *decodeTriangle(std::string in);

    // std::string encodeAAB(Shape *sh);

    AAB *decodeAAB(std::string in);
    
    void recalculateTriUVs(Triangle *tri);
    
    Material *usedMaterial;
    void usingMaterial(std::string name);
    bool isUsingMaterial() { return usedMaterial != NULL; };
    void endUsingMaterial() { usedMaterial = NULL; };
};

std::string encodeColour(Vec3 c);

Vec3 decodeColour(std::stringstream *in);

Material *emptyMaterial();

Bound *emptyBound();


Bound *boundByIndex(Container *c, int i);

CamPreset decodeCamPreset(std::string in);
std::string encodeCamPreset(CamPreset *p);


#endif
