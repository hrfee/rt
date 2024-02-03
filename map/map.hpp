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
};

struct RayResult {
    int collisions;
    float t0;
    float t1;
    Vec3 color;
};

class WorldMap {
    public:
        WorldMap(int width, int height, int depth): w(width), h(height), d(depth) {};
        WorldMap(char const* path);
        ~WorldMap();
        float w, h, d;
        std::vector<Sphere> spheres;
        std::vector<Triangle> triangles;
        Camera *cam;
        void appendSphere(Vec3 center, float radius, Vec3 color, float reflectiveness); 
        void appendTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float reflectiveness); 
        void castRays(Image *img, RenderConfig *rc);
        void encode(char const* path);
    private:
        RayResult castRay(Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount);
};

bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c);
#endif
