#ifndef MAP
#define MAP

#define MAX_BOUNCE 1000

#include <vector>
#include "../shape/shape.hpp"
#include "../cam/cam.hpp"
#include "../img/img.hpp"

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
        float w, h, d;
        std::vector<Sphere> spheres;
        Camera *cam;
        void appendSphere(Vec3 center, float radius, Vec3 color, float reflectiveness); 
        void castRays(Image *img);
        void encode(char const* path);
    private:
        RayResult castRay(Vec3 p0, Vec3 delta, int callCount);
};

#endif
