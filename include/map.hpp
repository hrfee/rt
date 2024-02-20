#ifndef MAP
#define MAP

#include <vector>
#include "shape.hpp"
#include "cam.hpp"
#include "img.hpp"

struct RenderConfig {
    bool renderOnChange;
    bool renderNow;
    bool lighting;
    bool reflections;
    bool specular;
    int maxBounce;
    float refractiveIndex;
    float distanceDivisor;
    float baseBrightness;
    float globalShininess;
    bool triangles, spheres;
    Vec3 manualPosition;
    bool planeOptimisation;
};

struct RayResult {
    int collisions;
    int potentialCollisions;
    float t;
    float t1;
    Vec3 color;
    Vec3 reflectionColor;
    Vec3 specularColor;
    Vec3 refractColor;
    float opacity;
    float reflectiveness;
    float emissiveness;
    float specular;
    float shininess;
    Vec3 emissionColor;
    Vec3 p0;
    Vec3 normal;
    Vec3 norm;
    Shape *obj;
};

class WorldMap {
    public:
        WorldMap(int width, int height, int depth): w(width), h(height), d(depth) { o.start = NULL; o.end = NULL; };
        WorldMap(char const* path);
        ~WorldMap();
        float w, h, d;
        float baseBrightness, globalShininess;
        void loadFile(char const* path);
        int loadObjFile(char const* path);
        std::vector<PointLight> pointLights;
        ContainerQuad o;
        ContainerQuad debug;
        Camera *cam;
        void createSphere(Vec3 center, float radius, Vec3 color, float opacity = 1.f, float reflectiveness = 0.f, float specular = 1.f, float shininess = -1.f);
        void createTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float opacity = 1.f, float reflectiveness = 0.f, float specular = 1.f, float shininess = -1.f); 
        void createDebugVector(Vec3 p0, Vec3 delta, Vec3 color = {1.f, 0.f, 0.f});
        void appendPointLight(Vec3 center, Vec3 color, float brightness);
        void castRays(Image *img, RenderConfig *rc);
        void encode(char const* path);
    private:
        RayResult castRay(ContainerQuad *c, Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount = 0);
        void castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount);
        void castShadowRays(Vec3 viewDelta, Vec3 p0, RenderConfig *rc, RayResult *res);
};

int clearContainer(ContainerQuad *c);

float meetsSphere(Vec3 p0, Vec3 delta, Sphere *sphere);
float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle *tri);
bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle *tri);
bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c);

void appendShape(ContainerQuad *c, Shape *sh);
void appendContainerQuad(ContainerQuad *cParent, ContainerQuad *c);
void appendSphere(ContainerQuad *c, Sphere *s);
void appendTriangle(ContainerQuad *c, Triangle *t);
#endif
