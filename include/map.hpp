#ifndef MAP
#define MAP

#include <sstream>
#include <vector>
#include "shape.hpp"
#include "cam.hpp"
#include "img.hpp"
#include "mat.hpp"
#include "tex.hpp"

extern const char *modes[3];

struct RenderConfig {
    int *threadStates;
    int nthreads;
    bool renderOnChange;
    bool renderNow;
    bool change;
    bool collisionsOnly;
    bool lighting;
    bool reflections;
    bool specular;
    int maxBounce;
    float refractiveIndex;
    float distanceDivisor;
    float baseBrightness;
    float globalShininess;
    bool triangles, spheres, aabs;
    bool mtTriangleCollision;
    bool normalMapping;
    bool reflectanceMapping;
    int samplesPerPx;
    int sampleMode;
    Vec3 manualPosition;
    // bool planeOptimisation;
    bool showDebugObjects;
};

struct RayResult {
    int collisions;
    // int potentialCollisions;
    float t;
    float t1;
    Vec3 color;
    Vec3 reflectionColor;
    Vec3 lightColor;
    Vec3 specularColor;
    Vec3 refractColor;
    Vec2 uv;
    Vec3 p0;
    Vec3 normal;
    Vec3 norm;
    Shape *obj;
    RayResult() { resetObj(); };
    bool hit() {
        return obj != NULL;
        // return collisions > 0;
    };
    void resetObj() {
        collisions = 0;
        // potentialCollisions = 0;
        t = 1e30f;
        t1 = 1e30f;
        color = {0,0,0};
        reflectionColor = {0,0,0};
        specularColor = {0,0,0};
        lightColor = {0,0,0};
        refractColor = {0,0,0};
        p0 = {0,0,0};
        normal = {0,0,0};
        norm = {0,0,0};
        obj = NULL;
    };
};

struct MapStats {
    std::string name;
    int spheres, tris, lights, planes, aabs, csgs, cylinders;
    int tex, norm;
    int missingTex, missingNorm, missingRef, missingObj;
    int allocs = 0;
    float moveSpeedMultiplier;
    void clear() {
        spheres = 0;
        tris = 0;
        lights = 0;
        planes = 0;
        aabs = 0;
        csgs = 0;
        cylinders = 0;
        tex = 0;
        norm = 0;
        missingTex = 0;
        missingNorm = 0;
        missingRef = 0;
        missingObj = 0;
        allocs = 0;
        moveSpeedMultiplier = 1.f;
        name.clear();
    }
    int size() {
        return lights + spheres + tris + planes + aabs + csgs + cylinders;
    }
};

class WorldMap {
    public:
        WorldMap(char const* path);
        ~WorldMap();
        float w, h, d;
        float baseBrightness, globalShininess;
        void loadFile(char const* path, double (*getTime)(void));
        void loadObjFile(char const* path, Mat4 transform = mat44Identity);
        void optimizeMap(double (*getTime)(void), int level = 1, int accelIdx = 1);
        int optimizeLevel;
        int accelIndex;
        bool bvh;
        int accelParam;
        float accelFloatParam;
        std::vector<PointLight> pointLights;
        std::vector<CamPreset> camPresets;
        const char **camPresetNames;
        Container unoptimizedObj;
        Container *obj;
        Container *flatObj;
        Container *optimizedObj;
        Container unoptimizable;
        char **objectNames;
        int objectCount;
        Shape **objectPtrs;
        
        TexStore tex;
        TexStore norms;
        TexStore refs;
        MaterialStore materials;
       
        Decoder dec;

        Image *aaOffsetImage;
        bool aaOffsetImageDirty;
        MapStats mapStats;
        Camera *cam;
        bool currentlyRendering;
        bool currentlyOptimizing;
        bool currentlyLoading;
        double lastRenderTime;
        double lastOptimizeTime;
        double lastLoadTime;
        void createSphere(Vec3 center, float radius, Vec3 color, float opacity = 1.f, float reflectiveness = 0.f, float specular = 1.f, float shininess = -1.f, float thickness = -1.f);
        void createTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float opacity = 1.f, float reflectiveness = 0.f, float specular = 1.f, float shininess = -1.f); 
        void createDebugVector(Vec3 p0, Vec3 delta, Vec3 color = {1.f, 0.f, 0.f});
        void appendPointLight(Vec3 center, Vec3 color, float brightness);
        double castRays(Image *img, RenderConfig *rc, double (*getTime)(void), int nthreads = -1);

        void encode(char const* path);

        void genObjectList(Container *c);
    private:
        void castRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount = 0);
        void castSubRays(Image *img, RenderConfig *rc, int w0, int w1, int h0, int h1, int *state, Vec2 *offsets, int nOffsets);
        void ray(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc);
        void traversalRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc);
        void voxelRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc);
        void castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount);
        void castShadowRays(Vec3 viewDelta, Vec3 p0, RenderConfig *rc, RayResult *res);
        void castThroughSphere(Vec3 delta, RenderConfig *rc, RayResult *res, int callCount = 0);
};

int clearContainer(Container *c);
void flattenRootContainer(Container *dst, Container *src, bool root = true);

float meetsSphere(Vec3 p0, Vec3 delta, Sphere *sphere);
float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle *tri);
bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle *tri);
bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c);

void appendSphere(Container *c, Sphere *s);
void appendTriangle(Container *c, Triangle *t);
#endif
