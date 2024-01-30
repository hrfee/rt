#include "map.hpp"

#include <cmath>

void appendSphere(WorldMap *m, Vec3 center, float radius, Vec3 color, float reflectiveness) {
    m->spheres.emplace_back(Sphere{ center, radius, color, reflectiveness });
}

void castRays(WorldMap *m, Image *img) {
    Vec3 baseRowVec = m->cam.viewportCorner;
    for (int y = 0; y < m->cam.h; y++) {
        // m->cam.viewportCorner is a vector from the origin, therefore all calculated pixel positions are.
        Vec3 delta = baseRowVec;
        for (int x = 0; x < m->cam.w; x++) {
            RayResult res = castRay(m, m->cam.position, delta, 0);
            if (res.collisions > 0) {
                writePixel(img, x, y, res.color);
            }
            
            delta = delta + m->cam.viewportCol; 
        }
        baseRowVec = baseRowVec + m->cam.viewportRow;
    }
}

RayResult castRay(WorldMap *m, Vec3 p0, Vec3 delta, int callCount) {
    RayResult res = {0, 9999.f, 9999.f};
    if (callCount > MAX_BOUNCE) return res;
    for (Sphere s: m->spheres) {
        // CG:PaP 2nd ed. in C, p. 703, eq. 15.17 is an expanded sphere equation with
        // substituted values for the camera position (x/y/z_0),
        // pixel vec from camera (delta x/y/z),
        // and normalized distance along pixel vector (t).
        Vec3 originToSphere = {p0.x - s.center.x, p0.y - s.center.y, p0.z - s.center.z};
        float b = 2.f * (delta.x*originToSphere.x + delta.y*originToSphere.y + delta.z*originToSphere.z);
        float a = dot(delta, delta);
        float c = dot(originToSphere, originToSphere) - s.radius*s.radius;

        float discrim = b*b - 4.f*a*c;
        float discrimRoot = std::sqrt(discrim);
        if (discrim > 0) {
            int collisions = 2;
            float t0 = (-b + discrimRoot) / (2.f*a);
            float t1 = (-b - discrimRoot) / (2.f*a);
            if (t0 < 0) {
                t0 = t1;
                collisions = 1;
            }
            if (t1 < 0) {
                if (t0 == t1) {
                    continue;
                }
                collisions = 1;
            }
            if (t0 >= 0 && t1 >= 0) {
                float t_0 = std::min(t0, t1);
                float t_1 = std::max(t0, t1);
                if (t_0 < res.t0) {
                    res.t0 = t_0;
                    res.color = s.color;
                }
                if (t_1 < res.t1) {
                    res.t1 = t_1;
                    res.color = s.color;
                }
            }
            res.collisions += collisions;

        } else if (discrim == 0) {
            int collisions = 1;
            float t0 = (-b + discrimRoot) / (2.f*a);
            if (t0 < 0) continue;
            if (t0 < res.t0) {
                res.t0 = t0;
                res.color = s.color;
            }
            res.collisions += collisions;
        } else {
            continue;
        }
        if (s.reflectiveness == 0) continue;
        Vec3 collisionPoint = p0 + (res.t0 * delta);
        RayResult bounce = castRay(m, collisionPoint, collisionPoint-s.center, callCount+1);
        Vec3 color = {0.f, 0.f, 0.f};
        if (bounce.collisions > 0) color = bounce.color;
        res.color = (1.f - s.reflectiveness)*s.color + (s.reflectiveness) * color;
    }
    return res;
}
