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
    Vec3 norms[2] = {cross(a - c, b - c), cross(c - a, b - a)};
    float components[2] = {0, 0};
    for (int i = 0; i < 2; i++) {
        // Roughly computing component of each normal along vector v0,
        // a.k.a (n_i \dot v0)/|v0|.
        // but since we just want the lowest one (most negative), the denom. can be ignored.
        components[i] = dot(norms[i], v0);
    }
    if (components[0] <= components[1])
        return norms[0];
    return norms[1];
}

float meetsSphere(Vec3 p0, Vec3 delta, Sphere *sphere) {
    // CG:PaP 2nd ed. in C, p. 703, eq. 15.17 is an expanded sphere equation with
    // substituted values for the camera position (x/y/z_0),
    // pixel vec from camera (delta x/y/z),
    // and normalized distance along pixel vector (t).
    Vec3 originToSphere = {p0.x - sphere->center.x, p0.y - sphere->center.y, p0.z - sphere->center.z};
    float a = dot(delta, delta);
    float b = 2.f * (delta.x*originToSphere.x + delta.y*originToSphere.y + delta.z*originToSphere.z);
    float c = dot(originToSphere, originToSphere) - sphere->radius*sphere->radius;
    float discrim = b*b - 4.f*a*c;
    if (discrim < 0) return -1;
    float discrimRoot = std::sqrt(discrim);
    float t = (-b - discrimRoot) / (2.f*a);
    if (discrim > 0 && t < 0) {
        float t1 = (-b + discrimRoot) / (2.f*a);
        if (t < 0) t = t1;
    }
    return t;
}

float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle *tri) {
    // dot product of a line on the plane with the normal is zero, hence (p - t.a) \dot norm = 0, where p is a random point (x, y, z)
    // p \dot norm = t.a \dot norm
    // compute the right side, expand the left side and subtract it:
    // (norm.x)x + (norm.y)y + (norm.z)z - (t.a \dot norm) = 0
    // Take values in equation of a plane (CGPaP in C p. 703 eq. 15.18): Ax + By + Cz + D = 0
    // where A = norm.x, B = norm.y, C = norm.z
    float d = -dot(tri->a, normal);
    // CGPaP in C p.703 eq. 15.21, using dot products to make more readable
    float t = -(dot(normal, p0) + d);
    float denom = dot(normal, delta);
    if (denom == 0) { // Plane parallel to ray, ignore
        return -1;
    }
    return t / denom;
}

bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle *tri) {
    // project orthographically as big as possible
    Vec2 ao, bo, co, po;
    if (normal.x >= normal.y && normal.x >= normal.z) {
        // std::printf("projecting onto x\n");
        ao = Vec2{tri->a.y, tri->a.z};
        bo = Vec2{tri->b.y, tri->b.z};
        co = Vec2{tri->c.y, tri->c.z};
        po = Vec2{collisionPoint.y, collisionPoint.z};
    } else if (normal.y >= normal.x && normal.y >= normal.z) {
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
