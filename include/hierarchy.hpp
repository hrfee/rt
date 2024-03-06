#ifndef HIERARCHY
#define HIERARCHY

#include "shape.hpp"
#include "vec.hpp"
#include <vector>

typedef void (&bvhSplitter)(Container *o, Container *a, Container *b, float *split, int *splitAxis, bool bvh, int lastAxis);

void splitSAH(Container *o, Container *a, Container *b, float *split, int *splitAxis, bool bvh, int lastAxis);
void splitEqually(Container *o, Container *a, Container *b, float *split, int *splitAxis, bool bvh, int lastAxis);

Container* generateHierarchy(Container *o, bvhSplitter split, bool bvh, int splitLimit, int splitCount = 0, int lastAxis = -1, int colorIndex = 0);
void printShapes(Container *c, int tabIndex = 0);

float containerSA(Container *o);
float shapeSA(Shape *sh);

void containerCube(Container *c, Vec3 color);
void containerSphereCorners(Container *c, Vec3 color);

Container* splitVoxels(Container *o, int subdivision);
void getVoxelIndex(Container *c, int subdivision, Vec3 p, Vec3 delta, int *x, int *y, int *z, float *t);

#endif
