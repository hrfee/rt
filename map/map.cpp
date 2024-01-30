#include "map.hpp"

#include <cmath>

void appendSphere(WorldMap *m, Vec3 center, float radius, Vec3 color) {
    m->spheres.emplace_back(Sphere{ center, radius, color });
}

void castRays(WorldMap *m, Image *img) {
    Vec3 baseRowVec = m->cam.viewportCorner;
    for (int y = 0; y < m->cam.h; y++) {
        // m->cam.viewportCorner is a vector from the origin, therefore all calculated pixel positions are.
        Vec3 delta = baseRowVec;
        for (int x = 0; x < m->cam.w; x++) {
            // CG:PaP 2nd ed. in C, p. 703, eq. 15.17 is an expanded sphere equation with
            // substituted values for the camera position (x/y/z_0),
            // pixel vec from camera (delta x/y/z),
            // and normalized distance along pixel vector (t).
            for (Sphere s: m->spheres) {
                Vec3 camToSphere = {m->cam.position.x - s.center.x, m->cam.position.y - s.center.y, m->cam.position.z - s.center.z};
                float b = 2.f * (delta.x*camToSphere.x + delta.y*camToSphere.y + delta.z*camToSphere.z);
                float a = dot(delta, delta);
                float c = dot(camToSphere, camToSphere) - s.radius*s.radius;
    
                float discrim = b*b - 4.f*a*c;
                float t1, t2;
                float discrimRoot = std::sqrt(discrim);
                if (discrim > 0) {
                    t1 = (-b + discrimRoot) / (2.f*a);
                    t2 = (-b - discrimRoot) / (2.f*a);
                } else if (discrim == 0) {
                    t1 = (-b + discrimRoot) / (2.f*a);
                    t2 = t1;
                } else {
                    continue;
                }
                writePixel(img, x, y, s.color);
            }

            delta = delta + m->cam.viewportCol; 
        }
        baseRowVec = baseRowVec + m->cam.viewportRow;
    }
}
