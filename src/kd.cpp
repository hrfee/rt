#include "kd.hpp"

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

Container* splitKD(Container *o, int splitLimit, int splitCount, int lastAxis) {
    if (splitCount >= splitLimit) return NULL;
    Container *kd = emptyContainer();
    kd->a = {9999, 9999, 9999};
    kd->b = {-9999, -9999, -9999};
    determineBounds(o, &(kd->a), &(kd->b));
    /*std::vector<Bound> v[3];
    v[0] = *o;
    v[1] = v[0];
    v[2] = v[0];*/

    /*// Sort shapes by increasing (axis: 0=x,1=y,2=z)
    std::sort(v[0].begin(), v[0].end(), sortByX);
    std::sort(v[1].begin(), v[1].end(), sortByY);
    std::sort(v[2].begin(), v[2].end(), sortByZ);*/

    float kda[3] = {kd->a.x, kd->a.y, kd->a.z};
    float kdb[3] = {kd->b.x, kd->b.y, kd->b.z};

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
                    ((bmin[i] >= kda[i]) || (bmax[i] >= kda[i])) &&
                    ((bmin[i] <= spl[i]) || (bmax[i] <= spl[i]))
                   ) {
                    h0++;
                }
                if (
                    ((bmin[i] >= spl[i]) || (bmax[i] >= spl[i])) &&
                    ((bmin[i] <= kdb[i]) || (bmax[i] <= kdb[i]))
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

    Container *containers[2] = {emptyContainer(), emptyContainer()};
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
            ((bmin[bestAxis] >= kda[bestAxis]) || (bmax[bestAxis] >= kda[bestAxis])) &&
            ((bmin[bestAxis] <= spl[bestAxis]) || (bmax[bestAxis] <= spl[bestAxis]))
           ) {
            appendToContainer(containers[0], *bo); // Make copy rather than appending existing pointer
            added = true;
        }
        if (
            ((bmin[bestAxis] >= spl[bestAxis]) || (bmax[bestAxis] >= spl[bestAxis])) &&
            ((bmin[bestAxis] <= kdb[bestAxis]) || (bmax[bestAxis] <= kdb[bestAxis]))
           ) {
            appendToContainer(containers[1], *bo); // Make copy rather than appending existing pointer
            added = true;
        }
        if (!added) std::printf("skipped!\n");
        bo = bo->next;
    }

    Shape *sections[2] = {emptyShape(), emptyShape()};

    std::srand(std::time(NULL)); // Uncomment when using debug spheres/cube
    for (int i = 0; i < 2;  i++) {
        Container *c = containers[i];
        if (c->size == 1) {
            std::memcpy(sections[i], c->start, sizeof(Shape));
            free(c);

        } else if (c->size != 0) {
            Container *splitAgain = splitKD(c, splitLimit, splitCount+1, bestAxis);
            if (splitAgain != NULL) {
                free(c);
                sections[i]->c = splitAgain;
            } else {
                sections[i]->c = c;
                c->a = kd->a;
                c->b = kd->b;
                c->splitAxis = bestAxis;

                Vec3 *boundary = &(c->b);
                if (i == 1) boundary = &(c->a);

                if (bestAxis == 0) boundary->x = spl[bestAxis];
                else if (bestAxis == 1) boundary->y = spl[bestAxis];
                else if (bestAxis == 2) boundary->z = spl[bestAxis];

                std::printf("split%d: (%f %f %f), (%f %f %f)\n", i, c->a.x, c->a.y, c->a.z, c->b.x, c->b.y, c->b.z); 
                // DEBUG SPHERES
                // containerSphereCorners(c);
                containerCube(c);
            }
            Bound bo;
            bo.s = sections[i];
            appendToContainer(kd, bo);
            // if (sections[i]->c->end == NULL) std::printf("got null with size %d!\n", splitGroups[i][bestIndex].size());
        }
    }

    /* if (bestIndex == 0) std::printf("best was x\n");
    else if (bestIndex == 1) std::printf("best was y\n");
    else std::printf("best was z\n");
    // Pick a point in the list, range is from from 0 - array[point+1]
    std::printf("group 1 (%f - %f):\n", kda[bestIndex], spl[bestIndex]);
    printShapes(splitGroups[0][bestIndex]);
    std::printf("group 2 (%f - %f):\n", spl[bestIndex], kdb[bestIndex]);
    printShapes(splitGroups[1][bestIndex]);

    std::printf("-----VS-----\n");*/

    if (splitCount == 0) {
        // std::printf("------INITIAL------\n");
        // printShapes(o);
        std::printf("------HIERARCHY (%d leaves)------\n", o->size);
        printShapes(kd);
    }
    return kd;
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

void containerCube(Container *c) {
    Vec3 color = {0.f, 0.f, 0.f};
    color.x = 0.5f + (float(std::rand() % 500) / 500.f);
    color.y = 0.5f + (float(std::rand() % 500) / 500.f);
    color.z = 0.5f + (float(std::rand() % 500) / 500.f);
    float width = 0.4f;
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
     // FIXME Draw line!
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
            sh0->t = t0;
            sh0->color = color;
            appendToContainer(c, sh0);
            Shape *sh1 = emptyShape();
            sh1->color = color;
            sh1->t = t1;
            appendToContainer(c, sh1);
        }
    }
}

void containerSphereCorners(Container *c) {
    Vec3 color = {0.f, 0.f, 0.f};
    color.x = 0.5f + (float(std::rand() % 500) / 500.f);
    color.y = 0.5f + (float(std::rand() % 500) / 500.f);
    color.z = 0.5f + (float(std::rand() % 500) / 500.f);
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
