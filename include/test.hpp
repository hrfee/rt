#ifndef TESTS
#define TESTS
#include "shape.hpp"

Triangle *triList(int n = 1000, int *seed = NULL);
Sphere *sphereList(int n = 1000, int *seed = NULL);

double traverseAll(Triangle *t, int n = 1000);
double traverseAll(Sphere *s, int n = 1000);

#endif
