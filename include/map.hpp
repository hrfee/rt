#ifndef MAP
#define MAP

#include <vector>
#include "shape.hpp"
#include "cam.hpp"
#include "img.hpp"
#include "mat.hpp"

struct RenderConfig {
    bool renderOnChange;
    bool renderNow;
    bool collisionsOnly;
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
    bool showDebugObjects;
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
    Vec3 p0;
    Vec3 normal;
    Vec3 norm;
    Shape *obj;
};

class WorldMap {
    public:
        WorldMap(int width, int height, int depth): w(width), h(height), d(depth), optimizedObj(NULL), optimizeLevel(0), splitterIndex(-1), camPresetNames(NULL), currentlyRendering(false), lastRenderTime(0.f) { obj = &unoptimizedObj; };
        WorldMap(char const* path);
        ~WorldMap();
        float w, h, d;
        float baseBrightness, globalShininess;
        void loadFile(char const* path);
        int loadObjFile(char const* path, Mat4 transform = mat44Identity);
        void optimizeMap(double (*getTime)(void), int level = 1, int splitterIndex = 1);
        int optimizeLevel;
        int splitterIndex;
        bool bvh;
        int splitterParam;
        std::vector<PointLight> pointLights;
        std::vector<CamPreset> camPresets;
        const char **camPresetNames;
        Container unoptimizedObj;
        Container *obj;
        Container *flatObj;
        Container *optimizedObj;
        Camera *cam;
        bool currentlyRendering;
        double lastRenderTime;
        double lastOptimizeTime;
        void createSphere(Vec3 center, float radius, Vec3 color, float opacity = 1.f, float reflectiveness = 0.f, float specular = 1.f, float shininess = -1.f, float thickness = -1.f);
        void createTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float opacity = 1.f, float reflectiveness = 0.f, float specular = 1.f, float shininess = -1.f); 
        void createDebugVector(Vec3 p0, Vec3 delta, Vec3 color = {1.f, 0.f, 0.f});
        void appendPointLight(Vec3 center, Vec3 color, float brightness);
        double castRays(Image *img, RenderConfig *rc, double (*getTime)(void), int nthreads = -1);
        void encode(char const* path);
    private:
        void castRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount = 0);
        void castSubRays(Image *img, RenderConfig *rc, int w0, int w1, int h0, int h1);
        void ray(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc);
        void traversalRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc);
        void voxelRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc);
        void castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount);
        void castShadowRays(Vec3 viewDelta, Vec3 p0, RenderConfig *rc, RayResult *res);
        void castThroughSphere(Vec3 delta, RenderConfig *rc, RayResult *res, int callCount = 0);
};

int clearContainer(Container *c);
void flattenRootContainer(Container *dst, Container *src);

float meetsSphere(Vec3 p0, Vec3 delta, Sphere *sphere);
float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle *tri);
bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle *tri);
bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c);

void appendSphere(Container *c, Sphere *s);
void appendTriangle(Container *c, Triangle *t);
#endif
