#ifndef MAP
#define MAP

#include <vector>
#include "../shape/shape.hpp"
#include "../cam/cam.hpp"
#include "../img/img.hpp"

struct WorldMap {
    float w, h, d;
    std::vector<Sphere> spheres;
    Camera cam;
};

struct RayResult {
    int collisions;
    float t0;
    float t1;
    Vec3 color;
};

void appendSphere(WorldMap *m, Vec3 center, float radius, Vec3 color); 
void castRays(WorldMap *m, Image *img);

RayResult castRay(WorldMap *m, Vec3 p0, Vec3 v);

#endif
