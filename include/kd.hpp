#ifndef KD
#define KD

#include "shape.hpp"
#include "vec.hpp"
#include <vector>

Container* splitKD(Container *o, int splitLimit, int splitCount = 0, int lastAxis = -1);
void printShapes(Container *kd, int tabIndex = 0);

void containerCube(Container *c);
void containerSphereCorners(Container *c);

#endif
