#ifndef HIERARCHY
#define HIERARCHY

#include "shape.hpp"
#include "vec.hpp"

extern const char *splitters[5];



// A bvhSplitter returns 1 if it believes a split should not occur.
typedef int (&bvhSplitter)(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int);

int splitSAH(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int);
int splitEqually(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int);
int splitBitree(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int maxNodesPerVox = 2);

Container* generateHierarchy(Container *o, int splitterIndex, bool bvh, int splitLimit, int splitCount = 0, int lastAxis = -1, int colorIndex = 0, int extra = 0);
Container* generateOctreeHierarchy(Container *o, int splitLimit, int splitCount, int colorIndex, int maxNodesPerVox);

    void printShapes(Container *c, int tabIndex = 0);

float containerSA(Container *o);
float shapeSA(Shape *sh);

void containerCube(Container *c, Vec3 color);
void containerSphereCorners(Container *c, Vec3 color);

Container* splitVoxels(Container *o, int subdivision);
void getVoxelIndex(Container *c, int subdivision, Vec3 p, Vec3 delta, int *x, int *y, int *z, float *t);

#endif
