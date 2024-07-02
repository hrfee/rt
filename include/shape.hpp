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
struct Bound;
class Container;
class AAB;
class Material;
class Shape;
class Cylinder;

namespace ShapeType {
    const int Sphere = 0;
    const int Triangle = 1;
    const int AAB = 2;
    const int CSG = 3;
    const int Cylinder = 4;


    const int Count = 5;
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
    Bound(Vec3 min = {0, 0, 0}, Vec3 max = {0, 0, 0}, Vec3 centroid = {0, 0, 0}, Shape *s = NULL, Bound *next = NULL):
        min(min),
        max(max),
        centroid(centroid),
        s(s),
        next(next)
    {};
    void reset() {
        min = {0, 0, 0}, max = {0, 0, 0}, centroid = {0, 0, 0};
        s = NULL;
        next = NULL;
    };
    // Bound with large min and low max to be used when find bounds of objects.
    static Bound forGrowing() {
        return Bound({1e30f, 1e30f, 1e30f}, {-1e30f, -1e30f, -1e30f}, {0, 0, 0});
    }
};

class Shape {
    public:
        Material *material;
        Transform transform;
        bool transformDirty;
        bool debug;
        Vec2 uvScale;
        Shape() {
            material = NULL;
            debug = false;
            uvScale = {1.f, 1.f};
            transformDirty = true;
            transform.reset();
        };
        Shape(Shape *sh) {
            material = sh->material;
            transform = sh->transform;
            transformDirty = sh->transformDirty;
            debug = sh->debug;
            uvScale = sh->uvScale;
        };
        virtual Shape *clone() { return NULL; };
        virtual Material *mat() { return material; };
        virtual Transform& trans() { return transform; };
        virtual void bounds(Bound *bo) = 0;
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL) = 0;
        virtual bool intersects(Vec3 p0, Vec3 delta) = 0;
        virtual void applyTransform() = 0;
        virtual void bakeTransform() = 0;
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1) = 0;
        virtual bool envelops(Vec3 /*min*/, Vec3 /*max*/) { return false; };
        virtual bool envelops(Bound& bo) { return envelops(bo.min, bo.max); };
        virtual int clear(bool /*deleteShapes*/ = true) { return 0; };
        virtual std::string name() = 0;
        virtual int type() = 0;
        virtual void flattenTo(Container *dst, bool root = true); // Defined in shape.cpp due to Container usage
        virtual Vec3 sampleTexture(Vec2 uv, Texture *tx);
        virtual void recalculateUVs(Texture* /*t*/ = NULL);
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

class Material {
    public:
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
        Material() {
            name = NULL;
            color = {0, 0, 0};
            texId = -1, normId = -1, refId = -1;
            noLighting = false;
            opacity = 1.f;
            reflectiveness = 0;
            specular = 1;
            shininess = -1.f;
            next = NULL;
        }
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
            faceForUV = 0;
            oMin = min;
            oMax = max;
        };
        AAB(Shape *sh): Shape(sh) {
            AAB *b = static_cast<AAB*>(sh);
            min = b->min;
            max = b->max;
            oMin = b->oMin;
            oMax = b->oMax;
            faceForUV = b->faceForUV;
        };
        virtual Shape *clone() {
            return new AAB(this);
        };

        int faceForUV;
        Vec2 getUV(Vec3 hit, Vec3 normal);
        AAB(Vec3 mn, Vec3 mx): min(mn), max(mx) {};
        virtual std::string name() { return std::string("Box"); };
        virtual int type() { return ShapeType::AAB; };
        virtual void bounds(Bound *bo);
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        virtual bool envelops(Vec3 mn, Vec3 mx);
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
        virtual void recalculateUVs(Texture *t = NULL);
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
        Container(Shape *sh): AAB(sh) {
            Container *c = static_cast<Container*>(sh);
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
        void grow(Bound *src);
        virtual void flattenTo(Container *dst, bool root = true);

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
        Sphere(Shape *sh): Shape(sh) {
            Sphere *s = static_cast<Sphere*>(sh);
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
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
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
        Triangle(Shape *sh): Shape(sh) {
            Triangle *t = static_cast<Triangle*>(sh);
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
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        // A triangle can't envelop a 3d object, so envelops() is left empty (returning false).
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
        virtual void recalculateUVs(Texture *t = NULL);
};

class CSG: public Shape {
    public:
        static const char *RelationNames[];
        static const int RelationCount = 4;
        enum Relation { Intersection = 0, Union = 1, Difference = 2, DifferenceHollowSubject = 3 };
        CSG::Relation relation;
        Shape *a, *b;

        CSG() {
            a = NULL, b = NULL;
            relation = Union;
        };
        CSG(Shape *sh): Shape(sh) {
            CSG *c = static_cast<CSG*>(sh);
            a = c->a->clone();
            b = c->b->clone();
            relation = c->relation;
        };
        virtual Shape *clone() {
            return new CSG(this);
        };
        virtual std::string name() { return std::string("CSG"); };
        virtual int type() { return ShapeType::CSG; };
        
        int append(Shape *sh);
        virtual Material *mat() { return a->mat(); };
        virtual void bounds(Bound *bo);
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual float intersectUnion(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual float intersectIntersection(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual float intersectDifference(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        virtual int clear(bool deleteShapes = true);
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
        virtual Vec3 sampleTexture(Vec2 uv, Texture *tx);
};

class Cylinder: public Shape {
    public:
        Vec3 center, oCenter;
        float radius, oRadius;
        float length, oLength;
        float thickness;
        int axis;

        Vec2 getUV(Vec3 hit);
        Cylinder() {
            center = {0.f, 0.f, 0.f};
            axis = 0;
            oCenter = center;
            radius = 0.f;
            oRadius = radius;
            length = 0.f;
            oLength = length;
            thickness = 1.f;
        }
        Cylinder(Shape *sh): Shape(sh) {
            Cylinder *cl = static_cast<Cylinder*>(sh);
            axis = cl->axis;
            center = cl->center;
            oCenter = cl->oCenter;
            radius = cl->radius;
            oRadius = cl->oRadius;
            length = cl->length;
            oLength = cl->oLength;
            thickness = cl->thickness;
        };
        virtual Shape *clone() {
            return new Sphere(this);
        };
        virtual std::string name() { return std::string("Cylinder"); };
        virtual int type() { return ShapeType::Cylinder; };
        virtual void bounds(Bound *bo);
        virtual float intersect(Vec3 p0, Vec3 delta, Vec3 *normal = NULL, Vec2 *uv = NULL, float *t1 = NULL, Vec3 *normal1 = NULL, Vec2 *uv1 = NULL);
        virtual bool intersects(Vec3 p0, Vec3 delta);
        virtual void applyTransform();
        virtual void bakeTransform();
        virtual void recalculateUVs(Texture* t = NULL);

        void refractSolid(float r0, float r1, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
        virtual void refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1);
};


struct PointLight {
    Vec3 center;
    Vec3 color;
    float brightness;
    // FIXME: Move specular cmpt to objects!
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
    
    Cylinder *decodeCylinder(std::string in);

    // std::string encodeTriangle(Shape *sh);

    Triangle *decodeTriangle(std::string in);

    // std::string encodeAAB(Shape *sh);

    AAB *decodeAAB(std::string in);
    
    void recalculateTriUVs(Triangle *tri);
    void recalculateAABUVs(AAB *box);
    
    Material *usedMaterial;
    void usingMaterial(std::string name);
    bool isUsingMaterial() { return usedMaterial != NULL; };
    void endUsingMaterial() { usedMaterial = NULL; };

    Texture *getFirstTexture(Material *m);

};

std::string encodeColour(Vec3 c);

Vec3 decodeColour(std::stringstream *in);

CamPreset decodeCamPreset(std::string in);
std::string encodeCamPreset(CamPreset *p);


#endif
