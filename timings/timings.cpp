#include "test.hpp"
#include "shape.hpp"
#include <getopt.h>
#include <cstdio>
#include <cfloat>

#define REPEAT 1000

int main(int argc, char **argv) {
    int n = 1000;
    int flag;
    while ((flag = getopt(argc, argv, "n:")) != -1) {
        switch (flag) {
            case 'n':
              n = std::stoi(std::string(optarg));
              break;
        }
    }
    std::printf("Initializing shape lists of size %d\n", n);
    int seed = -9998;
    Triangle *t = triList(n, &seed);
    Sphere *s = sphereList(n, &seed);
    
    double tTa = 0.f, tSa = 0.f, tTmin = DBL_MAX, tSmin = DBL_MAX, tTmax = -DBL_MAX, tSmax = -DBL_MAX;
    for (int i = 0; i < REPEAT; i++) {
        double tri = traverseAll(t, n);
        tTa += tri;
        if (tri < tTmin) tTmin = tri;
        if (tri > tTmax) tTmax = tri;
        double sphere = traverseAll(s, n);
        tSa += sphere;
        if (sphere < tSmin) tSmin = sphere;
        if (sphere > tSmax) tSmax = sphere;
        std::printf("Loop %d completed\n", i);
    }
    tTa /= float(REPEAT);
    tSa /= float(REPEAT);

    double tRa = tTa/tSa, tRmin = tTmin/tSmin, tRmax = tTmax/tSmax;
    std::printf("Triangles: min(%f), avg(%f), max(%f)\n", tTmin, tTa, tTmax);
    std::printf("Spheres: min(%f), avg(%f), max(%f)\n", tSmin, tSa, tSmax);
    std::printf("Ratio tri/sphere: min(%f), avg(%f), max(%f)\n", tRmin, tRa, tRmax);

    std::printf("%d & Triangle & %f & %f & %f\\\\\n\\midrule\n", n, tTmin, tTa, tTmax);
    std::printf("%d & Sphere   & %f & %f & %f\\\\\n\\midrule\n", n, tSmin, tSa, tSmax);
    std::printf("%d & \\makecell{$\\frac{Triangle}{Sphere}$} & %f & %f & %f\\\\\n\\midrule\n", n, tRmin, tRa, tRmax);
}

