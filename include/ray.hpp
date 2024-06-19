#ifndef RAY
#define RAY

#include "vec.hpp"
#include "shape.hpp"

bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c);

Vec3 getVisibleTriNormal(Vec3 v0, Vec3 a, Vec3 b, Vec3 c);

float meetsSphere(Vec3 p0, Vec3 delta, Sphere *sphere, float *t1 = NULL);
float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle *tri);
bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle *tri);
float meetsTriangleMT(Vec3 p0, Vec3 delta, Triangle *tri, Vec3 *bary);
Vec2 triUV(Vec3 bary, Triangle *tri);
bool meetsAABB(Vec3 p0, Vec3 delta, Container *container);
float meetAABB(Vec3 p0, Vec3 delta, Vec3 a, Vec3 b);
float meetAABBWithNormal(Vec3 p0, Vec3 delta, Vec3 a, Vec3 b, Vec3 *normal);
Vec2 aabUV(Vec3 p0, AAB *aab, Vec3 normal);
Vec2 sphereUV(Sphere *s, Vec3 p);

Vec3 Refract(float ra, float rb, Vec3 a, Vec3 normal, bool *tir);

#endif
