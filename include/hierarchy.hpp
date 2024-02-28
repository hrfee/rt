#ifndef HIERARCHY
#define HIERARCHY

#include "shape.hpp"
#include "vec.hpp"
#include <vector>

typedef float (&bvhSplitter)(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis);

float splitSAH(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis);
float splitEqually(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis);

Container* splitBVH(Container *o, bvhSplitter split, int splitLimit, int splitCount = 0, int lastAxis = -1, int colorIndex = 0);
void printShapes(Container *c, int tabIndex = 0);

float containerSA(Container *o);
float shapeSA(Shape *sh);

void containerCube(Container *c, Vec3 color);
void containerSphereCorners(Container *c, Vec3 color);

#endif
