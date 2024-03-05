#ifndef HIERARCHY
#define HIERARCHY

#include "shape.hpp"
#include "vec.hpp"
#include <vector>

typedef float (&bvhSplitter)(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis, float* bvhSpl2);

float splitSAH(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis, float *bvhSpl2);
float splitEqually(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis, float *bvlSph2);

Container* splitKdBvhHybrid(Container *o, bvhSplitter split, bool bvh, int splitLimit, int splitCount = 0, int lastAxis = -1, int colorIndex = 0);
void printShapes(Container *c, int tabIndex = 0);

float containerSA(Container *o);
float shapeSA(Shape *sh);

void containerCube(Container *c, Vec3 color);
void containerSphereCorners(Container *c, Vec3 color);

Container* splitVoxels(Container *o, int subdivision);
void getVoxelIndex(Container *c, int subdivision, Vec3 p, Vec3 delta, int *x, int *y, int *z, float *t);

#endif
