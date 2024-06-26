#include "ray.hpp"

#include "mat.hpp"

#include <cmath>

// FIXME: My own derivation better explains this implementation, variable names should be adjusted.
Vec3 Refract(float ra, float rb, Vec3 a, Vec3 normal, bool *tir) {
    // Based on CGPaP in C p.757-758 Sect. 16.5.2 "Calculating the refraction vector":
    // \vec{I} = a_,
    // \vec{T} = out,
    // \vec{N} = normal_,
    
    Vec3 a_ = 1.f * norm(a);
    Vec3 normal_ = norm(normal); 
    *tir = false;
    float indexRatio = ra / rb;
    float cosIncident = -dot(normal_, a_);
    // When the square root below (cosOut) is imaginary, TIR occurs.
    // Hence when sinIncidentSquared is greater than 1, TIR occurs.
    float sinIncidentSquared = (indexRatio * indexRatio) * (1.f - (cosIncident*cosIncident));
    if (sinIncidentSquared > 1.f) {
        *tir = true;
        return {0.f, 0.f, 0.f};
    }
    float cosOut = std::sqrt(1.f - sinIncidentSquared);

    Vec3 out = (((indexRatio * cosIncident) - cosOut) * normal_) + (indexRatio * a_);
    return out;
}

/*bool meetsAABB(Vec3 p0, Vec3 delta, Container *container) {

    if (delta.y < 0.f) return true;
    Vec3 tMin = (container->a - p0);
    tMin = {tMin.x / delta.x, tMin.y / delta.y, tMin.z / delta.z};
    Vec3 tMax = (container->b - p0);
    tMax = {tMax.x / delta.x, tMax.y / delta.y, tMax.z / delta.z};
    if (mag(tMin) > mag(tMax)) {
        Vec3 tm = tMin;
        tMin = tMax;
        tMax = tm;
    }
    // tMin/tMax now correlate to the nearest and farthest planes of the box
    float near = std::max(std::max(tMin.x, tMin.y), tMin.z);
    float far = std::min(std::min(tMax.x, tMax.y), tMax.z);
    return near <= far;
}*/

/*
bool meetsAABB(Vec3 p0, Vec3 delta, Container *container) {
    return meetAABB(p0, delta, container->a, container->b) < -9990.f ? false : true;
}

float meetAABB(Vec3 p0, Vec3 delta, Vec3 a, Vec3 b) {
    // Based on the "slab method".
    // Find the distance along (delta) you'd have to travel to hit the planes of each pair of parallel faces,
    // then select the largest distance to one of the nearer faces, and the shortest distance to one of the furthest faces.
    // If the latter is negative, we're looking in the wrong direction.
    // Where the ray actually intersects the AABB, the latter (furthest) should be greater than the former (nearest).
    // and therefore if a nearer face appears to be closer than a farther face, we do not intersect.
    
    // Pre-divide, since multiplication is faster and we have a loop
    Vec3 d = {1.f/delta.x, 1.f/delta.y, 1.f/delta.z};
    float tmin = -9999.f;
    float tmax = 9999.f;
    for (int i = 0; i < 3; i++) {
        float t0 = (a(i) - p0(i)) * d(i);
        float t1 = (b(i) - p0(i)) * d(i);
        tmin = std::max(tmin, std::min(t0, t1));
        tmax = std::min(tmax, std::max(t0, t1));
    }

    if (tmax < 0) return -9999.f;
    if (tmin > tmax) return -9998.f;
    return tmin;
}


float meetAABBWithNormal(Vec3 p0, Vec3 delta, Vec3 a, Vec3 b, Vec3 *normal) {
    // Based on the "slab method".
    // Find the distance along (delta) you'd have to travel to hit the planes of each pair of parallel faces,
    // then select the largest distance to one of the nearer faces, and the shortest distance to one of the furthest faces.
    // If the latter is negative, we're looking in the wrong direction.
    // Where the ray actually intersects the AABB, the latter (furthest) should be greater than the former (nearest).
    // and therefore if a nearer face appears to be closer than a farther face, we do not intersect.
    
    // Pre-divide, since multiplication is faster and we use it twice
    float d[3] = {1.f/delta.x, 1.f/delta.y, 1.f/delta.z};
    float tmin = -9999.f;
    float tmax = 9999.f;
    int nminIdx = -1, nmaxIdx = -1;
    for (int i = 0; i < 3; i++) {
        float t0 = (a(i) - p0(i)) * d[i];
        float t1 = (b(i) - p0(i)) * d[i];
        float mn = std::min(t0, t1);
        if (mn >= tmin) {
            tmin = mn;
            nminIdx = i;
        }
        float mx = std::max(t0, t1);
        if (mx <= tmax) {
            tmax = mx;
            nmaxIdx = i;
        }
    }

    if (tmax < 0) return -9999.f;
    if (tmin > tmax) return -9998.f;
    *normal = {0.f, 0.f, 0.f};
    // Inside the box
    if (tmin < 0.f) {
        normal->idx(nmaxIdx) = delta(nmaxIdx) >= 0 ? -1.f : 1.f;
        return tmax;
    // Outside the box
    } else {
        normal->idx(nminIdx) = delta(nminIdx) >= 0 ? -1.f : 1.f;
        return tmin;
    }
    // normal->idx(normIdx) = 1.f;
}

Vec2 aabUV(Vec3 p0, AAB *aab, Vec3 normal) {
    int uvIdx = 0;
    Vec2 uv = {0.f, 0.f};
    for (int i = 0; i < 3; i++) {
        if (normal(i) != 0.f) continue;
        uv(uvIdx) = (p0(i) - aab->min(i)) / (aab->max(i) - aab->min(i));
        uvIdx++;
    }
    return uv;
}

Vec2 sphereUV(Sphere *s, Vec3 p) {
    // "Move" the sphere (and the point on it) to center O.
    p = p - s->center;
    // Get polar angles from O
    float theta = std::atan2(p.x, p.z);
    float phi = std::acos(p.y / s->radius);

    // Divide by 360deg so range is 1,
    float rU = theta / (2.f*M_PI);

    return Vec2{
        1.f - (rU + 0.5f), // and shift upwards so between 0 & 1.
        1.f - (phi / float(M_PI))
    };
}

*/
