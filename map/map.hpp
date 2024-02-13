#ifndef MAP
#define MAP

#define MAX_BOUNCE 1000

#include <vector>
#include "../shape/shape.hpp"
#include "../cam/cam.hpp"
#include "../img/img.hpp"

struct RenderConfig {
    bool renderOnChange;
    bool renderNow;
    bool lighting;
    bool reflections;
    bool specular;
    float distanceDivisor;
    float baseBrightness;
    float globalShininess;
    bool triangles, spheres;
    Vec3 manualPosition;
};

struct RayResult {
    int collisions;
    float t;
    Vec3 color;
    float reflectiveness;
    float emissiveness;
    float specular;
    float shininess;
    Vec3 emissionColor;
    Vec3 p0;
    Vec3 normal;
};

class WorldMap {
    public:
        WorldMap(int width, int height, int depth): w(width), h(height), d(depth), start(NULL), end(NULL) {};
        WorldMap(char const* path);
        ~WorldMap();
        float w, h, d;
        float baseBrightness, globalShininess;
        void loadFile(char const* path);
        void loadObjFile(char const* path);
        std::vector<PointLight> pointLights;
        Shape *start;
        Shape *end;
        Camera *cam;
        void appendSphere(Sphere *s);
        void appendSphere(Vec3 center, float radius, Vec3 color, float reflectiveness, float specular, float shininess = -1.f);
        void appendTriangle(Triangle *t);
        void appendTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float reflectiveness, float specular, float shininess = -1.f); 
        void appendPointLight(Vec3 center, Vec3 color, float brightness);
        void castRays(Image *img, RenderConfig *rc);
        void encode(char const* path);
    private:
        RayResult castRay(Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount = 0);
        void castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount);
        void castShadowRays(Vec3 viewDelta, Vec3 p0, RenderConfig *rc, RayResult *res);

        void appendShape(Shape *sh);
        void clearShapes();
};


float meetsSphere(Vec3 p0, Vec3 delta, Sphere sphere);
float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle tri);
bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle tri);
bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c);
#endif
