#ifndef ACCEL
#define ACCEL

#include "shape.hpp"
#include "vec.hpp"

extern const char *accelerators[5];

namespace Accel {
    const int DivideObjectsEqually = 0;
    const int SAH = 1;
    const int Voxel = 2;
    const int BiTree = 3;
    const int FalseOctree = 4;
    const int None = -1;
}

// A bvhSplitter returns 1 if it believes a split should not occur.
typedef int (&bvhSplitter)(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int);

int splitSAH(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis = 0, float costTriSphereRatio = 1.5f);
int splitEqually(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int);
int splitBitree(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int maxNodesPerVox = 2);

Container* generateHierarchy(Container *o, int accel, bool bvh, int splitLimit, int splitCount = 0, int lastAxis = -1, int colorIndex = 0, int extra = 0, float fextra = 0.f);
Container* generateOctreeHierarchy(Container *o, int splitLimit, int splitCount, int colorIndex, int maxNodesPerVox, int parentId = 1);

    void printShapes(Container *c, int tabIndex = 0);

float containerSA(Container *o);
float shapeSA(Shape *sh);

void containerCube(Container *c, Vec3 color);
void containerSphereCorners(Container *c, Vec3 color);

Container* splitVoxels(Container *o, int subdivision);
void getVoxelIndex(Container *c, int subdivision, Vec3 p, Vec3 delta, int *x, int *y, int *z, float *t);

#endif
