#include "hierarchy.hpp"

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "shape.hpp"
#include "vec.hpp"

bool sortByX(Bound a, Bound b) {
    Bound *bo[2] = {&a, &b};
    float mins[2] = {9999.f, 9999.f};
    for (int i = 0; i < 2; i++) {
        if (bo[i]->s->s != NULL) {
            mins[i] = bo[i]->s->s->center.x - bo[i]->s->s->radius;
        } else if (bo[i]->s->t != NULL) {
            mins[i] = std::min(std::min(bo[i]->s->t->a.x, bo[i]->s->t->b.x), bo[i]->s->t->c.x);
        }
    }
    return mins[0] < mins[1];
}

bool sortByY(Bound a, Bound b) {
    Bound *bo[2] = {&a, &b};
    float mins[2] = {9999.f, 9999.f};
    for (int i = 0; i < 2; i++) {
        if (bo[i]->s->s != NULL) {
            mins[i] = bo[i]->s->s->center.y - bo[i]->s->s->radius;
        } else if (bo[i]->s->t != NULL) {
            mins[i] = std::min(std::min(bo[i]->s->t->a.y, bo[i]->s->t->b.y), bo[i]->s->t->c.y);
        }
    }
    return mins[0] < mins[1];
}

bool sortByZ(Bound a, Bound b) {
    Bound *bo[2] = {&a, &b};
    float mins[2] = {9999.f, 9999.f};
    for (int i = 0; i < 2; i++) {
        if (bo[i]->s->s != NULL) {
            mins[i] = bo[i]->s->s->center.z - bo[i]->s->s->radius;
        } else if (bo[i]->s->t != NULL) {
            mins[i] = std::min(std::min(bo[i]->s->t->a.z, bo[i]->s->t->b.z), bo[i]->s->t->c.z);
        }
    }
    return mins[0] < mins[1];
}

void flattenContainer(Container *o, std::vector<Bound> *v) {
    Bound *bo = o->start;
    while (bo != o->end->next) {
        Shape *current = bo->s;
        if (current->c != NULL) {
            std::vector<Bound> cv;
            flattenContainer(current->c, &cv);
            v->insert(v->end(), cv.begin(), cv.end());
        } else {
            Bound b;
            b.s = current;
            if (current->s != NULL) {
                b.min = current->s->center - current->s->radius;
                b.max = current->s->center + current->s->radius;
            } else if (current->t != NULL) {
                b.min = {9999, 9999, 9999};
                b.max = {-9999, -9999, -9999};
                minPerComponent(&(b.min), current->t->a);
                maxPerComponent(&(b.max), current->t->a);
                minPerComponent(&(b.min), current->t->b);
                maxPerComponent(&(b.max), current->t->b);
                minPerComponent(&(b.min), current->t->c);
                maxPerComponent(&(b.max), current->t->c);
            }
            v->push_back(b);
        }
        bo = bo->next;
    }
}

void determineBounds(Container *o, Vec3 *min, Vec3 *max) {
    Bound *bo = o->start;
    while (bo != o->end->next) {
        Shape *current = bo->s;
        if (current->t != NULL) {
            minPerComponent(min, current->t->a);
            maxPerComponent(max, current->t->a);
            minPerComponent(min, current->t->b);
            maxPerComponent(max, current->t->b);
            minPerComponent(min, current->t->c);
            maxPerComponent(max, current->t->c);
        } else if (current->s != NULL) {
            minPerComponent(min, current->s->center - current->s->radius);
            maxPerComponent(max, current->s->center + current->s->radius);
        }
        bo = bo->next;
    }
}

namespace {
    // Line generated with distincColors.py
    Vec3 distinctColors[16] = {{0.545, 0.271, 0.075}, {0.098, 0.098, 0.439}, {0.000, 0.502, 0.000}, {0.741, 0.718, 0.420}, {0.690, 0.188, 0.376}, {1.000, 0.000, 0.000}, {1.000, 0.647, 0.000}, {1.000, 1.000, 0.000}, {0.486, 0.988, 0.000}, {0.000, 0.980, 0.604}, {0.000, 1.000, 1.000}, {0.000, 0.000, 1.000}, {1.000, 0.000, 1.000}, {0.392, 0.584, 0.929}, {0.933, 0.510, 0.933}, {0.902, 0.902, 0.980}};

}

float shapeSA(Shape *sh) {
    if (sh->t != NULL) {
        Vec3 ab = sh->t->b - sh->t->a;
        float mAB = mag(ab);
        Vec3 ac = sh->t->c - sh->t->a;
        float mAC = mag(ac);
        float d = dot(ab, ac);
        return 0.5f * std::sqrt(mAB*mAB*mAC*mAC - d*d);
    } else if (sh->s != NULL) {
        return 4.f * M_PI * (sh->s->radius*sh->s->radius);
    } else if (sh->c != NULL) {
        return containerSA(sh->c);
    }
    return 0.f;
}


float containerSA(Container *o) {
    float sa = 0.f;
    Bound *bo = o->start;
    while (bo != o->end->next) {
        Shape *current = bo->s;
        float s = shapeSA(current);
        sa += s;
        bo = bo->next;
    }
    return sa;
}

// splitSAH: Use the SAH heuristic:
// "MACDONALD J. D., BOOTH K. S.: Heuristics for Ray Tracing Using Space Subdivision."
// As discovered in review paper:
// "M. HAPALA, V. HAVRAN: Review: Kd-tree Traversal Algorithms for Ray Tracing"
// Based on the idea that rays intersecting an object is ~proportional to it's surface area.
float splitSAH(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis) {
    // NOTES
    // Cost function given in paper IS suitable for now, as we do not yet cache intersections per ray (i.e. if leaf is in two AABBs, we currently test it twice).
    // Also see notes in sah.md
    
    // Assumed cost values (C_o)
    float cSphere = 1.f;
    float cTri = 2.f;
    // Assumed cost of traversing an interior node (C_i)
    float cTraverse = 1.f;

    float min[3] = {minCorner.x, minCorner.y, minCorner.z};
    float max[3] = {maxCorner.x, maxCorner.y, maxCorner.z};

    float spl[3];

    int bestIndices[3] = {0, 0, 0};
    float bestCosts[3] = {9999, 9999, 9999};

    for (int i = 0; i < 3; i++) {
        int splitIndex = 0;
        Bound *bsplit = o->start;
        while (bsplit != o->end->next) {
            Vec3 split = bsplit->max;
            if (i == 0) spl[i] = split.x;
            else if (i == 1) spl[i] = split.y;
            else if (i == 2) spl[i] = split.z;
            float sa = 0.f, sa0 = 0.f, sa1 = 0.f;
            // int h = 0;
            int h0 = 0, h1 = 0;
            Bound *bo = o->start;
            while (bo != o->end->next) {
                float s = shapeSA(bo->s);
                sa += s;
                // h++;
                float bmin[3] = {bo->min.x, bo->min.y, bo->min.z}; 
                float bmax[3] = {bo->max.x, bo->max.y, bo->max.z};
                if (
                    ((bmin[i] >= min[i]) || (bmax[i] >= min[i])) &&
                    ((bmin[i] <= spl[i]) || (bmax[i] <= spl[i]))
                   ) {
                    sa0 += s;
                    h0++;
                }
                if (
                    ((bmin[i] >= spl[i]) || (bmax[i] >= spl[i])) &&
                    ((bmin[i] <= max[i]) || (bmax[i] <= max[i]))
                   ) {
                    sa1 += s;
                    h1++;
                }
                bo = bo->next;
            }
            
            // FIXME: Adapt to use sphere and triangle costs!
            float cost = cTraverse + cTri * (h0*(sa0/sa) + h1*(sa1/sa));

            if (cost < bestCosts[i]) {
                bestIndices[i] = splitIndex;
                bestCosts[i] = cost;
            }
            splitIndex++;
            bsplit = bsplit->next;
        }
    }

    int bestAxis = 0;
    float bestCost = 9999.f;
    for (int i = 0; i < 3; i++) {
        /* if (i == lastAxis) {
            continue;
        } */
        if (bestCosts[i] < bestCost) {
            bestAxis = i;
            bestCost = bestCosts[i];
        }
    }

    *splitAxis = bestAxis;

    Vec3 split = boundByIndex(o, bestIndices[bestAxis])->max;
    if (bestAxis == 0) spl[bestAxis] = split.x;
    else if (bestAxis == 1) spl[bestAxis] = split.y;
    else if (bestAxis == 2) spl[bestAxis] = split.z;
    Bound *bo = o->start;
    while (bo != o->end->next) {
        float bmin[3] = {bo->min.x, bo->min.y, bo->min.z}; 
        float bmax[3] = {bo->max.x, bo->max.y, bo->max.z};
        bool added = false;
        if (
            ((bmin[bestAxis] >= min[bestAxis]) || (bmax[bestAxis] >= min[bestAxis])) &&
            ((bmin[bestAxis] <= spl[bestAxis]) || (bmax[bestAxis] <= spl[bestAxis]))
           ) {
            appendToContainer(a, *bo); // Make copy rather than appending existing pointer
            added = true;
        }
        if (
            ((bmin[bestAxis] >= spl[bestAxis]) || (bmax[bestAxis] >= spl[bestAxis])) &&
            ((bmin[bestAxis] <= max[bestAxis]) || (bmax[bestAxis] <= max[bestAxis]))
           ) {
            appendToContainer(b, *bo); // Make copy rather than appending existing pointer
            added = true;
        }
        if (!added) std::printf("skipped!\n");
        bo = bo->next;
    }
    return spl[bestAxis];
}

// splitEqually: Heuristic find split that best divides the -number- of elements between the two children.
// sets splitAxis to the axis split on, and returns the float value of where that occurs on that axis.
float splitEqually(Container *o, Container *a, Container *b, Vec3 minCorner, Vec3 maxCorner, int *splitAxis, int lastAxis) {
    float min[3] = {minCorner.x, minCorner.y, minCorner.z};
    float max[3] = {maxCorner.x, maxCorner.y, maxCorner.z};

    float spl[3];

    int bestIndices[3] = {0, 0, 0};
    int bestSizes[3] = {9999, 9999, 9999};
    float bestRatios[3] = {-9999, -9999, -9999};

    for (int i = 0; i < 3; i++) {
        int splitIndex = 0;
        Bound *bsplit = o->start;
        while (bsplit != o->end->next) {
            Vec3 split = bsplit->max;
            if (i == 0) spl[i] = split.x;
            else if (i == 1) spl[i] = split.y;
            else if (i == 2) spl[i] = split.z;
            int h0 = 0, h1 = 0;
            Bound *bo = o->start;
            while (bo != o->end->next) {
                float bmin[3] = {bo->min.x, bo->min.y, bo->min.z}; 
                float bmax[3] = {bo->max.x, bo->max.y, bo->max.z};
                if (
                    ((bmin[i] >= min[i]) || (bmax[i] >= min[i])) &&
                    ((bmin[i] <= spl[i]) || (bmax[i] <= spl[i]))
                   ) {
                    h0++;
                }
                if (
                    ((bmin[i] >= spl[i]) || (bmax[i] >= spl[i])) &&
                    ((bmin[i] <= max[i]) || (bmax[i] <= max[i]))
                   ) {
                    h1++;
                }
                bo = bo->next;
            }
            if (h0 > h1) {
                int h_ = h0;
                h0 = h1;
                h1 = h_;
            }
            float ratio = float(h0)/float(h1);
            if ((ratio <= 1 && ratio > bestRatios[i])) {// && (h0+h1) <= bestSize) {
                bestIndices[i] = splitIndex;
                bestRatios[i] = ratio;
                bestSizes[i] = h0 + h1;
            }
            splitIndex++;
            bsplit = bsplit->next;
        }
    }
    int bestAxis = 0;
    int bestSize = 9999;
    float bestRatio = -9999.f;
    for (int i = 0; i < 3; i++) {
        if (i == lastAxis) {
            continue;
        }
        if ((bestRatios[i] <= 1 && bestRatios[i] > bestRatio) && bestSizes[i] <= bestSize) {
            bestAxis = i;
            bestSize = bestSizes[i];
        }
    }

    *splitAxis = bestAxis;

    Vec3 split = boundByIndex(o, bestIndices[bestAxis])->max;
    if (bestAxis == 0) spl[bestAxis] = split.x;
    else if (bestAxis == 1) spl[bestAxis] = split.y;
    else if (bestAxis == 2) spl[bestAxis] = split.z;
    Bound *bo = o->start;
    while (bo != o->end->next) {
        float bmin[3] = {bo->min.x, bo->min.y, bo->min.z}; 
        float bmax[3] = {bo->max.x, bo->max.y, bo->max.z};
        bool added = false;
        if (
            ((bmin[bestAxis] >= min[bestAxis]) || (bmax[bestAxis] >= min[bestAxis])) &&
            ((bmin[bestAxis] <= spl[bestAxis]) || (bmax[bestAxis] <= spl[bestAxis]))
           ) {
            appendToContainer(a, *bo); // Make copy rather than appending existing pointer
            added = true;
        }
        if (
            ((bmin[bestAxis] >= spl[bestAxis]) || (bmax[bestAxis] >= spl[bestAxis])) &&
            ((bmin[bestAxis] <= max[bestAxis]) || (bmax[bestAxis] <= max[bestAxis]))
           ) {
            appendToContainer(b, *bo); // Make copy rather than appending existing pointer
            added = true;
        }
        if (!added) std::printf("skipped!\n");
        bo = bo->next;
    }
    return spl[bestAxis];
}

Container* splitBVH(Container *o, bvhSplitter split, int splitLimit, int splitCount, int lastAxis, int colorIndex) {
    if (splitCount >= splitLimit) return NULL;
    Container *out = emptyContainer();
    out->a = {9999, 9999, 9999};
    out->b = {-9999, -9999, -9999};
    determineBounds(o, &(out->a), &(out->b));
    /*std::vector<Bound> v[3];
    v[0] = *o;
    v[1] = v[0];
    v[2] = v[0];*/

    /*// Sort shapes by increasing (axis: 0=x,1=y,2=z)
    std::sort(v[0].begin(), v[0].end(), sortByX);
    std::sort(v[1].begin(), v[1].end(), sortByY);
    std::sort(v[2].begin(), v[2].end(), sortByZ);*/

    

    Container *containers[2] = {emptyContainer(), emptyContainer()};
    int bestAxis = 0;

    // SPLITTING ALGORITHM/HEURISTIC
    // float spl = splitEqually(o, containers[0], containers[1], out->a, out->b, &bestAxis, lastAxis);
    float spl = split(o, containers[0], containers[1], out->a, out->b, &bestAxis, lastAxis);

    Shape *sections[2] = {emptyShape(), emptyShape()};
   
    for (int i = 0; i < 2;  i++) {
        Container *c = containers[i];
        if (c->size == 1) {
            std::memcpy(sections[i], c->start, sizeof(Shape));
            free(c);

        } else if (c->size != 0) {
            Container *splitAgain = splitBVH(c, split, splitLimit, splitCount+1, bestAxis, colorIndex);
            colorIndex += (splitLimit - splitCount)*2;
            if (splitAgain != NULL) {
                free(c);
                c = splitAgain;
            } else {
                c->a = out->a;
                c->b = out->b;
                c->splitAxis = bestAxis;

                Vec3 *boundary = &(c->b);
                if (i == 1) boundary = &(c->a);

                if (bestAxis == 0) boundary->x = spl;
                else if (bestAxis == 1) boundary->y = spl;
                else boundary->z = spl;
            }
            // DEBUG SPHERES
            // containerSphereCorners(c);
            containerCube(c, distinctColors[colorIndex]);
            colorIndex++;
            
            sections[i]->c = c;
            Bound bo;
            bo.s = sections[i];
            appendToContainer(out, bo);
        }
    }

    // Uncomment to render hierarchy in detail in console.
    /* if (splitCount == 0) {
        // std::printf("------INITIAL------\n");
        // printShapes(o);
        std::printf("------HIERARCHY (%d leaves)------\n", o->size);
        printShapes(out);
    } */
    return out;
}

#define C_BLUE "\x1b[34m"
#define C_GREEN "\x1b[32m"
#define C_RESET "\x1b[0m"

void printShapes(Container *c, int tabIndex) {
    auto prefix = std::string(tabIndex*2, ' ');
    Bound *bo = c->start;
    while (bo != c->end->next) {
        Shape *current = bo->s;
        if (current->c != NULL) {
            char splitAxis = 'x';
            if (current->c->splitAxis == 1) splitAxis = 'y';
            else if (current->c->splitAxis == 2) splitAxis = 'z';
            std::printf("%s%snode(%d%c, (%.3f, %.3f %.3f), (%.3f, %.3f, %.3f)): {%s\n",
                    prefix.c_str(), C_BLUE, current->c->size, splitAxis,
                    current->c->a.x, current->c->a.y, current->c->a.z,
                    current->c->b.x, current->c->b.y, current->c->b.z,
                    C_RESET);
            printShapes(current->c, tabIndex+1);
            std::printf("%s%s}%s;\n", prefix.c_str(), C_BLUE, C_RESET);
        } else {
            std::printf("%s%sleaf%s ", prefix.c_str(), C_GREEN, C_RESET);
            if (current->s != NULL) {
                std::printf("sphere radius(%.3f) center(%.3f, %.3f, %.3f) ptr(%p) ", current->s->radius, current->s->center.x, current->s->center.y, current->s->center.z, current->s);
            } else if (current->t != NULL) {
                std::printf("triangle ");
            }
            std::printf("color(%.3f, %.3f, %.3f);\n", current->color.x, current->color.y, current->color.z);
        }
        bo = bo->next;
    }
}

void containerCube(Container *c, Vec3 color) {
    /* Vec3 color = {0.f, 0.f, 0.f};
    color.x = 0.5f + (float(std::rand() % 500) / 500.f);
    color.y = 0.5f + (float(std::rand() % 500) / 500.f);
    color.z = 0.5f + (float(std::rand() % 500) / 500.f); */
    float width = 0.2f;
    float pts[2][3] = {{c->a.x, c->a.y, c->a.z}, {c->b.x, c->b.y, c->b.z}};
    Vec3 vtx[8];
    int idx = 0;
    for (int i = 0; i < 2;  i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                vtx[idx] = {pts[i][0], pts[j][1], pts[k][2]};
                idx++;
            }
        }
    }
    // Worked out by hand
    int edges[12][2] = {{0, 1}, {0, 2}, {2, 3}, {1, 3}, {4, 6}, {4, 5}, {7, 5}, {7, 6}, {0, 4}, {1, 5}, {2, 6}, {7, 3}};
    for (int i = 0; i < 12; i++) {
        Vec3 ab[2] = {vtx[edges[i][0]], vtx[edges[i][1]]};
        Vec3 d = ab[1] - ab[0];
        int notAxis = (d.x != 0) ? 0 : (d.y != 0) ? 1 : 2;
        int axes[2];
        int j_ = 0;
        for (int j = 0; j < 3; j++) {
            if (j == notAxis) continue;
            axes[j_] = j;
            j_++;
        }

        float a[3] = {ab[0].x, ab[0].y, ab[0].z};
        float b[3] = {ab[1].x, ab[1].y, ab[1].z};
        for (int j = 0; j < 2; j++) {
            float corners[4][3] = {
                {a[0], a[1], a[2]},
                {a[0], a[1], a[2]},
                {b[0], b[1], b[2]},
                {b[0], b[1], b[2]}
            };
            corners[0][axes[j]] += width;
            corners[1][axes[j]] -= width;
            corners[2][axes[j]] -= width;
            corners[3][axes[j]] += width;
            Triangle *t0 = (Triangle*)malloc(sizeof(Triangle));
            t0->a = Vec3{corners[0][0], corners[0][1], corners[0][2]};
            t0->b = Vec3{corners[1][0], corners[1][1], corners[1][2]};
            t0->c = Vec3{corners[2][0], corners[2][1], corners[2][2]};
            Triangle *t1 = (Triangle*)malloc(sizeof(Triangle));
            t1->a = Vec3{corners[2][0], corners[2][1], corners[2][2]};
            t1->b = Vec3{corners[3][0], corners[3][1], corners[3][2]};
            t1->c = Vec3{corners[0][0], corners[0][1], corners[0][2]};
            Shape *sh0 = emptyShape();
            sh0->debug = true;
            sh0->t = t0;
            sh0->color = color;
            appendToContainer(c, sh0);
            Shape *sh1 = emptyShape();
            sh1->debug = true;
            sh1->color = color;
            sh1->t = t1;
            appendToContainer(c, sh1);
        }
    }
}

void containerSphereCorners(Container *c, Vec3 color) {
    /* Vec3 color = {0.f, 0.f, 0.f};
    color.x = 0.5f + (float(std::rand() % 500) / 500.f);
    color.y = 0.5f + (float(std::rand() % 500) / 500.f);
    color.z = 0.5f + (float(std::rand() % 500) / 500.f); */
    std::vector<Shape*> debugShapes;
    for (int j = 0; j < 2; j++) {
        Bound bo;
        Shape *dsh = emptyShape();
        bo.s = dsh;
        dsh->s = (Sphere*)malloc(sizeof(Sphere));
        dsh->s->center = (j == 0) ? c->a : c->b;
        dsh->s->radius = 0.3f;
        dsh->s->thickness = 1.f;
        dsh->color = color;
        dsh->opacity = 1.f;
        dsh->reflectiveness = 0.f;
        dsh->specular = 0.f;
        dsh->shininess = 5.f;
        appendToContainer(c, bo);
    }
}
