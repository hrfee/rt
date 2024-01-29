#ifndef MAP
#define MAP

#include <vector>
#include "../shape/shape.hpp"

struct WorldMap {
    float w, h, d;
    std::vector<Sphere> spheres;
};

void appendSphere(WorldMap *m, Vec3 center, float radius, Vec3 color); 

#endif
