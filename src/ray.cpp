#include "ray.hpp"

#include "mat.hpp"

#include <cmath>

// CGPaP in C p.339 Sect. 7.12.2 "PtInPolygon":
// Cast a ray from our point in any direction.
// If the ray hits an edge once, we're inside. If twice, we're outside.
// Uses cramer's rule to find point where ray meets edge.
bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    // std::printf("tri %f %f %f %f %f %f\n", a.x, a.y, b.x, b.y, c.x, c.y);
    // std::printf("point %f %f\n", p.x, p.y);
    int collisions = 0;
    float maxX = std::fmax(std::fmax(a.x, b.x), c.x);
    float minX = std::fmin(std::fmin(a.x, b.x), c.x);
    float maxY = std::fmax(std::fmax(a.y, b.y), c.y);
    float minY = std::fmin(std::fmin(a.y, b.y), c.y);
    // Easy case: point is outside triangle's bounding box.
    // These aren't full-fledged polygons so they aren't gonna have
    // a hole in the middle.
    if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY) {
        return false;
    }
    // Cast a ray across the x axis, p + q, where q=(maxX+1, p.y) - p:
    Vec2 q = Vec2{maxX+1.f, p.y} - p;
    Vec2 edges[3][2] = {{a, b}, {b, c}, {c, a}};
    for (int i = 0; i < 3; i++) {
        Vec2 r = edges[i][0];
        Vec2 s = edges[i][1] - r;
        // Find where p + tq = r + us, using cramer's rule:
        Mat2 d = Mat2{
            q.x, -s.x,
            q.y, -s.y
        };
        Mat2 dt = Mat2{
            r.x - p.x, -s.x,
            r.y - p.y, -s.y
        };
        Mat2 du = Mat2{
            q.x, r.x - p.x,
            q.y, r.y - p.y
        };
        float detD = det(d);
        float t = det(dt)/detD;
        float u = det(du)/detD;
        if (detD != 0 && t >= 0.f && u >= 0.f && t <= 1.f && u <= 1.f) {
            // Vec2 g = p + (t * q);
            // Vec2 j = r + (u * s);
            // std::printf("collision on %d ((%f %f) + t(%f %f) = (%f %f) + u(%f %f))! t=%f (%f, %f), u=%f (%f, %f)\n", i, p.x, p.x, q.x, q.y, r.x, r.y, s.x, s.y, t, g.x, g.y, u, j.x, j.y);
            collisions += 1;
        }
        if (collisions == 2) break;
    }
    // if (collisions == 1) {
    //     std::printf("in triangle!\n");
    // }
    return collisions == 1;
}

// Finds the normal of a triangle (abc) that points towards given vector p0
Vec3 getVisibleTriNormal(Vec3 v0, Vec3 a, Vec3 b, Vec3 c) {
    /* Vec3 norms[2] = {cross(a - c, b - c), cross(c - a, b - a)};
    float components[2] = {0, 0};
    for (int i = 0; i < 2; i++) {
        // Roughly computing component of each normal along vector v0,
        // a.k.a (n_i \dot v0)/|v0|.
        // but since we just want the lowest one (most negative), the denom. can be ignored.
        components[i] = dot(norms[i], v0);
    }
    if (components[0] <= components[1])
        return norms[0];
    return norms[1]; */
    Vec3 norm = cross(c-a, b-a);
    if (dot(norm, v0) <= 0.f) return norm;
    else return cross(a-c, b-c);
}

float meetsSphere(Vec3 p0, Vec3 delta, Sphere *sphere, float *t1) {
    // CG:PaP 2nd ed. in C, p. 703, eq. 15.17 is an expanded sphere equation with
    // substituted values for the camera position (x/y/z_0),
    // pixel vec from camera (delta x/y/z),
    // and normalized distance along pixel vector (t).
    Vec3 originToSphere = p0 - sphere->center;
    float a = dot(delta, delta);
    float b = 2.f * dot(delta, originToSphere);
    float c = dot(originToSphere, originToSphere) - sphere->radius*sphere->radius;
    float discrim = b*b - 4.f*a*c;
    if (discrim < 0) return -1;
    float discrimRoot = std::sqrt(discrim);
    float denom = 0.5f / a;
    float t = (-b - discrimRoot) * denom;
    if (discrim > 0 && t < 0) {
        float t_1 = (-b + discrimRoot) * denom;
        float t_ = 0.f;
        if (t < 0 || (t > t_1 && t_1 > 0)) {
            t_ = t;
            t = t_1;
            t_1 = t_;
        }
        if (t1 != NULL) {
            *t1 = t_1;
        }
    }
    return t;
}

float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle *tri) {
    // dot product of a line on the plane with the normal is zero, hence (p - t.a) \dot norm = 0, where p is a random point (x, y, z)
    // p \dot norm = t.a \dot norm
    // (p0 + t(delta)) \dot norm = t.a \dot norm
    // t(delta \dot norm) = (t.a - p0) \dot norm
    // divide and get t!
    float denom = dot(normal, delta);
    if (denom == 0) { // Plane parallel to ray, ignore
        return -1;
    }
    return dot((tri->a - p0), normal) / denom;
}

// Faster triangle collision algorithm which calculates barycentric coordinates to determine t.
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
float meetsTriangleMT(Vec3 p0, Vec3 delta, Triangle *tri, Vec3 *bary) {
    Vec3 e1 = tri->b - tri->a;
    Vec3 e2 = tri->c - tri->a;
    Vec3 rayEdge = cross(delta, e2);
    float det = dot(e1, rayEdge);

    // Uncomment for sided-ness
    // if (det <= 0.f) return -1.f;

    float invDet = 1.f / det;
    Vec3 ap0 = p0 - tri->a;
    // u
    bary->x = invDet * dot(ap0, rayEdge);

    if (bary->x < 0.f || bary->x > 1.f) return -1.f;

    Vec3 ap0Edge = cross(ap0, e1);
    // v
    bary->y = invDet * dot(delta, ap0Edge);

    if (bary->y < 0.f || bary->x + bary->y > 1.f) return -1.f;

    float t = invDet * dot(e2, ap0Edge);

    // w
    bary->z = 1.f - bary->x - bary->y;

    return t;
}

Vec2 triUV(Vec3 bary, Triangle *tri) {
    // FIXME: These obviously aren't right.
    return bary.z*tri->UVs[0] + bary.x*tri->UVs[1] + bary.y*tri->UVs[2];
}

bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle *tri) {
    // Sign doesn't matter here as our triangles aren't single-faced.
    Vec3 absNormal = {std::abs(normal.x), std::abs(normal.y), std::abs(normal.z)};
    // project orthographically as big as possible
    Vec2 ao, bo, co, po;
    if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z) {
        // std::printf("projecting onto x\n");
        ao = Vec2{tri->a.y, tri->a.z};
        bo = Vec2{tri->b.y, tri->b.z};
        co = Vec2{tri->c.y, tri->c.z};
        po = Vec2{collisionPoint.y, collisionPoint.z};
    } else if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z) {
        // std::printf("projecting onto y\n");
        ao = Vec2{tri->a.x, tri->a.z};
        bo = Vec2{tri->b.x, tri->b.z};
        co = Vec2{tri->c.x, tri->c.z};
        po = Vec2{collisionPoint.x, collisionPoint.z};
    } else { 
        // std::printf("projecting onto z\n");
        ao = Vec2{tri->a.x, tri->a.y};
        bo = Vec2{tri->b.x, tri->b.y};
        co = Vec2{tri->c.x, tri->c.y};
        po = Vec2{collisionPoint.x, collisionPoint.y};
    }

    return pointInTriangle(po, ao, bo, co);
}

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
