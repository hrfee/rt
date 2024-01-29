#include "map.hpp"

void appendSphere(WorldMap *m, Vec3 center, float radius, Vec3 color) {
    m->spheres.emplace_back(Sphere{ center, radius, color });
}
