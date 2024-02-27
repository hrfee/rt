#ifndef KD
#define KD

#include "shape.hpp"
#include "vec.hpp"
#include <vector>

Container* splitKD(Container *o, int splitLimit, int splitCount = 0, int lastAxis = -1, int colorIndex = 0);
void printShapes(Container *kd, int tabIndex = 0);

void containerCube(Container *c, Vec3 color);
void containerSphereCorners(Container *c, Vec3 color);

#endif
