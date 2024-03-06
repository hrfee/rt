#include "hierarchy.hpp"

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "shape.hpp"
#include "vec.hpp"
#include "ray.hpp"

bool sortByX(Bound a, Bound b) {
    Bound *bo[2] = {&a, &b};
    float mins[2] = {1e30f, 1e30f};
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
    float mins[2] = {1e30f, 1e30f};
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
    float mins[2] = {1e30f, 1e30f};
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
                b.centroid = current->s->center;
            } else if (current->t != NULL) {
                b.min = {1e30, 1e30, 1e30};
                b.max = {-1e30, -1e30, -1e30};
                minPerComponent(&(b.min), current->t->a);
                maxPerComponent(&(b.max), current->t->a);
                minPerComponent(&(b.min), current->t->b);
                maxPerComponent(&(b.max), current->t->b);
                minPerComponent(&(b.min), current->t->c);
                maxPerComponent(&(b.max), current->t->c);
                b.centroid = 0.333f * (current->t->a + current->t->b + current->t->c);
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

int whichSide(float bmin, float bmax, float a, float mid, float b) {
    bool left = false, right = false;
    if (
        ((bmin >= a) || (bmax >= a)) &&
        ((bmin <= mid) || (bmax <= mid))
       ) {
           left = true;
    }
    if (
        ((bmin >= mid) || (bmax >= mid)) &&
        ((bmin <= b) || (bmax <= b))
       ) {
        right = true;
    }
    if (left && right) return 2;
    if (left) return 0;
    if (right) return 1;
    return -1;
}

int whichSideCentroid(float centroid, float a, float mid, float b) {
    return centroid <= mid ? 0 : 1;
}

// splitSIROM: Uses the Scene-interior ray origin metric
// "BARTOSZ FABIANOWSKI et al.: A Cost Metric for Scene-Interior Ray Origins"
void splitSIROM(Container *o, float *split, int *splitAxis, bool bvh, int lastAxis) {
}

void growBound(Bound *dst, Bound *src) {
    minPerComponent(&(dst->min), src->min);
    maxPerComponent(&(dst->max), src->max);
}

float aabbSA(Vec3 a, Vec3 b) {
    Vec3 d = b - a;
    return 2.f*(d.x*d.y + d.x*d.z + d.y*d.z);
}

// splitSAH: Use the SAH heuristic:
// "MACDONALD J. D., BOOTH K. S.: Heuristics for Ray Tracing Using Space Subdivision."
// As discovered in review paper:
// "M. HAPALA, V. HAVRAN: Review: Kd-tree Traversal Algorithms for Ray Tracing"
// Based on the idea that rays intersecting an object is ~proportional to it's surface area.
int splitSAH(Container *o, float *split, int *splitAxis, bool bvh, int lastAxis) {
    // NOTES
    // Cost function given in paper IS suitable for now, as we do not yet cache intersections per ray (i.e. if leaf is in two AABBs, we currently test it twice).
    // Also see notes in sah.md
    
    // Assumed cost values (C_o)
    float cSphere = 1.f;
    float cTri = 1.5f;
    // Assumed added cost of traversing the extra nodes added when splitting (C_i)
    float cTraverse = 1.f;

    Vec3 spl;

    int bestIndices[3] = {0, 0, 0};
    float bestCosts[3] = {1e30f, 1e30f, 1e30f};

    // float *surfaceAreas = (float*)malloc(sizeof(float)*o->size);
    int shapeCount[2] = {0, 0};
    bool countComplete = false;

    for (int i = 0; i < 3; i++) {
        int splitIndex = 0;
        Bound *bsplit = o->start;
        while (bsplit != o->end->next) {
            Vec3 split = bvh ? bsplit->centroid : bsplit->max;
            spl(i) = split(i);
            // index 0 = spheres, 1 = triangles
            // float sa0[2] = {0.f, 0.f}, sa1[2] = {0.f, 0.f};
            Bound potentialSplit[2];
            std::memset(&potentialSplit, 0, sizeof(Bound)*2);
            potentialSplit[0].min = {1e30f, 1e30f, 1e30f};
            potentialSplit[0].max = {-1e30f, -1e30f, -1e30f};
            std::memcpy(&(potentialSplit[1]), &(potentialSplit[0]), sizeof(Bound));
            // int h = 0;
            int h0[2] = {0, 0}, h1[2] = {0, 0};
            Bound *bo = o->start;
            while (bo != o->end->next) {
                int shapeIndex = (bo->s->t != NULL) ? 1 : 0;
                /*if (!saCalculated) {
                    surfaceAreas[j]= shapeSA(bo->s);
                    totalSA[shapeIndex] += surfaceAreas[j];
                    shapeCount[shapeIndex]++;
                }*/
// h++;
                if (!countComplete) {
                    shapeCount[shapeIndex]++;
                }
                int side = -1;
                if (bvh) {
                    side = whichSideCentroid(bo->centroid(i), o->a(i), spl(i), o->b(i));
                } else {
                    side = whichSide(bo->min(i), bo->max(i), o->a(i), spl(i), o->b(i));
                }
                if (side == 0 || side == 2) {
                    growBound(&(potentialSplit[0]), bo);
                    // sa0[shapeIndex] += surfaceAreas[j];
                    h0[shapeIndex]++;
                }
                if (side == 1 || side == 2) {
                    growBound(&(potentialSplit[1]), bo);
                    // sa1[shapeIndex] += surfaceAreas[j];
                    h1[shapeIndex]++;
                }
                bo = bo->next;
            }
            countComplete = true;
            // saCalculated = true;
            
            // We don't need to divide by totalSA, since its the same for all compared values
            
            float sa[2] = {aabbSA(potentialSplit[0].min, potentialSplit[0].max), aabbSA(potentialSplit[1].min, potentialSplit[1].max)};
            if (h0[0] + h0[1] == 0) {
                sa[0] = 0.f;
            }
            if (h1[0] + h1[1] == 0) {
                sa[1] = 0.f;
            }

            float cost = cTraverse + (sa[0]*(h0[0]*cSphere + h0[1]*cTri)) + (sa[1]*(h1[0]*cSphere + h1[1]*cTri));
            // float cost = cTraverse + (cSphere * (h0[0]*(sa0[0]) + h1[0]*(sa1[0]))) + (cTri * (h0[1]*(sa0[1]) + h1[1]*(sa1[1])));
            // float cost = cTraverse + (cSphere * (h0[0]*(sa0[0]) + h1[0]*(sa1[0]))) + (cTri * (h0[1]*(sa0[1]) + h1[1]*(sa1[1])));
            // cost /= (totalSA[0] + totalSA[1]);
            // float parentCost = (cSphere * (h0[0]+h1[0])*(sa0[0]+sa1[0])) + (cTri * (h0[1]+h1[1])*(sa0[1]+sa1[1]));
            // std::printf("h0(%d), h1(%d), cost(%d): %f\n", h0[0] + h0[1], h1[0] + h1[1], splitIndex, cost);
            if (cost < bestCosts[i]) {
                bestIndices[i] = splitIndex;
                bestCosts[i] = cost;
            }
            splitIndex++;
            bsplit = bsplit->next;
        }
    }

    // free(surfaceAreas);

    int bestAxis = 0;
    float bestCost = 1e30f;
    for (int i = 0; i < 3; i++) {
        // Splitting on the same axis is fine since we have an actual heuristic.
        /* if (i == lastAxis) {
            continue;
        } */
        if (bestCosts[i] < bestCost) {
            bestAxis = i;
            bestCost = bestCosts[i];
        }
    }

    // Determine the cost of just not traversing the given node unsplit, and check if it's any better.
    // float noSplitCost = (cSphere*shapeCount[0]*totalSA[0]) + (cTri*shapeCount[1]*totalSA[1]);
    float noSplitCost = aabbSA(o->a, o->b)*(cSphere*shapeCount[0] + cTri*shapeCount[1]);
    if (bestCost >= noSplitCost) {
        return 1;
    }
    *splitAxis = bestAxis;

    Bound *splitOn = boundByIndex(o, bestIndices[bestAxis]);
    *split = (bvh ? splitOn->centroid : splitOn->max)(bestAxis);
    return 0;
}

// splitEqually: Heuristic find split that best divides the -number- of elements between the two children.
// sets splitAxis to the axis split on, and returns the float value of where that occurs on that axis.
int splitEqually(Container *o, float *split, int *splitAxis, bool bvh, int lastAxis) {
    Vec3 spl;

    int bestIndices[3] = {0, 0, 0};
    int bestSizes[3] = {9999, 9999, 9999};
    float bestRatios[3] = {-1e30, -1e30, -1e30};

    for (int i = 0; i < 3; i++) {
        int splitIndex = 0;
        Bound *bsplit = o->start;
        while (bsplit != o->end->next) {
            spl(i) = bsplit->max(i);
            int h0 = 0, h1 = 0;
            Bound *bo = o->start;
            while (bo != o->end->next) {
                int side = -1;
                if (bvh) {
                    side = whichSideCentroid(bo->centroid(i), o->a(i), spl(i), o->b(i));
                } else {
                    side = whichSide(bo->min(i), bo->max(i), o->a(i), spl(i), o->b(i));
                }
                if (side == 0 || side == 2) {
                    h0++;
                }
                if (side == 1 || side == 2) {
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
    int bestSize = 1e9;
    float bestRatio = -1e30f;
    for (int i = 0; i < 3; i++) {
        // Don't split on the same axis as before, no real logic behind this but nor is there for this heuristic.
        if (i == lastAxis) {
            continue;
        }
        if ((bestRatios[i] <= 1 && bestRatios[i] > bestRatio) && bestSizes[i] <= bestSize) {
            bestAxis = i;
            bestSize = bestSizes[i];
        }
    }

    *splitAxis = bestAxis;

    Bound *splitOn = boundByIndex(o, bestIndices[bestAxis]);
    *split = (bvh ? splitOn->centroid : splitOn->max)(bestAxis);
    
    return 0;
}


// Not really either, as space is partitioned on a plane (like KD-trees), but splits are constrained to the boundaries of their contents, like BVHs.
Container* generateHierarchy(Container *o, bvhSplitter split, bool bvh, int splitLimit, int splitCount, int lastAxis, int colorIndex) {
    if (splitCount >= splitLimit) return NULL;
    Container *out = emptyContainer();
    out->a = {1e30, 1e30, 1e30};
    out->b = {-1e30, -1e30, -1e30};
    determineBounds(o, &(out->a), &(out->b));
    o->a = out->a;
    o->b = out->b;
    /*std::vector<Bound> v[3];
    v[0] = *o;
    v[1] = v[0];
    v[2] = v[0];*/

    /*// Sort shapes by increasing (axis: 0=x,1=y,2=z)
    std::sort(v[0].begin(), v[0].end(), sortByX);
    std::sort(v[1].begin(), v[1].end(), sortByY);
    std::sort(v[2].begin(), v[2].end(), sortByZ);*/

    

    int bestAxis = 0;

    // SPLITTING ALGORITHM/HEURISTIC
    // float spl = splitEqually(o, containers[0], containers[1], out->a, out->b, &bestAxis, lastAxis);
    float splA = -1.f;
    int stopSplitting = split(o, &splA, &bestAxis, bvh, lastAxis);
    if (stopSplitting) {
        std::printf("stopped splitting @ %d\n", splitCount);
        free(out);
        return NULL;
    }
    
    Container *containers[2] = {emptyContainer(), emptyContainer()};
    
    // If doing a BVH, this will soon store the actual edges of the volumes, since there isn't a shared "split" edge.
    float bbEdges[2] = {splA, splA};
    // std::printf("got split axis %d, spl1 %f spl2 %f\n", bestAxis, spl, spl2);

    Bound *bo = o->start;
    while (bo != o->end->next) {
        bool added = false;
        int side = -1;
        if (bvh) {
            side = whichSideCentroid(bo->centroid(bestAxis), o->a(bestAxis), splA, o->b(bestAxis));
        } else {
            side = whichSide(bo->min(bestAxis), bo->max(bestAxis), o->a(bestAxis), splA, o->b(bestAxis));
        }
        if (side == 0 || side == 2) {
            appendToContainer(containers[0], *bo); // Make copy rather than appending existing pointer
            added = true;
            if (bvh) {
                bbEdges[0] = std::max(bbEdges[0], bo->max(bestAxis));
            }
        }
        if (side == 1 || side == 2) {
            appendToContainer(containers[1], *bo); // Make copy rather than appending existing pointer
            added = true;
            if (bvh) {
                bbEdges[1] = std::min(bbEdges[1], bo->min(bestAxis));
            }
        }
        if (!added) std::printf("skipped!\n");
        bo = bo->next;
    }

    Shape *sections[2] = {emptyShape(), emptyShape()};
   
    for (int i = 0; i < 2;  i++) {
        Container *c = containers[i];
        /*if (c->size == 1) {
            std::memcpy(sections[i], c->start, sizeof(Shape));
            free(c);

        } else */if (c->size != 0) {
            Container *splitAgain = generateHierarchy(c, split, bvh, splitLimit, splitCount+1, bestAxis, colorIndex);
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

                // FIXME: Use vector indexing!
                if (!bvh || i == 0) {
                    boundary->idx(bestAxis) = bbEdges[0];
                } else {
                    boundary->idx(bestAxis) = bbEdges[1];
                }
            }
            // DEBUG SPHERES
            // containerSphereCorners(c);
            containerCube(c, distinctColors[colorIndex % 16]);
            colorIndex++;
            
            sections[i]->c = c;
            Bound bo;
            bo.min = c->a;
            bo.max = c->b;
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
    float width = 0.02f;
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

// Unused/Broken Voxel stuff.

void getVoxelIndex(Container *c, int subdivision, Vec3 p, Vec3 delta, int *x, int *y, int *z, float *t) {
    Vec3 dims = {c->b.x-c->a.x, c->b.y-c->a.y, c->b.z-c->a.z};
    Vec3 vox = dims / subdivision;
    // std::printf("got vox(%f %f %f\n", vox.x, vox.y, vox.z);
    Vec3 ap = (p - c->a);
    if (!(ap.x < 0 || ap.x > dims.x || ap.y < 0 || ap.y > dims.y || ap.z < 0 || ap.z > dims.z)) {
        Vec3 apVox = {ap.x/vox.x, ap.y/vox.y, ap.z/vox.z};
        *x = apVox.x;
        *y = apVox.y;
        *z = apVox.z;
        return;
    }
    *t = meetAABB(ap, delta, {0.f, 0.f, 0.f}, dims);
    if (*t < -9990.f) {
        *x = -1;
        *y = -1;
        *z = -1;
        return;
    }
    ap = p + (*t * delta) - c->a;
    Vec3 apVox = {ap.x/vox.x, ap.y/vox.y, ap.z/vox.z};
    // std::printf("got t=%f, %f %f %f\n", t, apVox.x, apVox.y, apVox.z);
    *x = apVox.x;
    *y = apVox.y;
    *z = apVox.z;
}

// Split container into subcontainer "voxel" grid of dimensions subdivision^3.
// "start"/"end" is a pointer to start of a subdivision^3 array.
Container* splitVoxels(Container *o, int subdivision) {
    Container *out = emptyContainer();
    out->a = {1e30, 1e30, 1e30};
    out->b = {-1e30, -1e30, -1e30};
    out->voxelSubdiv = subdivision;
    determineBounds(o, &(out->a), &(out->b));
    Vec3 dims = {out->b.x-out->a.x, out->b.y-out->a.y, out->b.z-out->a.z};
    Vec3 vox = dims / subdivision;
    std::printf("global dim(%f %f %f), vox(%f %f %f)\n", dims.x, dims.y, dims.z, vox.x, vox.y, vox.z);
    size_t totalSize = subdivision*subdivision*subdivision;
    out->size = totalSize;
    Bound *grid = (Bound*)malloc(sizeof(Bound)*totalSize);
    std::memset(grid, 0, sizeof(Bound)*totalSize);
    appendToContainer(out, grid);
    for (int z = 0; z < subdivision; z++) {
        for (int y = 0; y < subdivision; y++) { 
            for (int x = 0; x < subdivision; x++) { 
                Bound *b = grid + x + subdivision * (y + subdivision * z);
                b->s = emptyShape();
                b->s->c = emptyContainer();
                Container *c = b->s->c;
                c->a = {vox.x * x, vox.y * y, vox.z * z};
                c->b = c->a + vox;
                b->min = c->a;
                b->max = c->b;
                float min[3] = {c->a.x, c->a.y, c->a.z};
                float max[3] = {c->b.x, c->b.y, c->b.z};
                c->splitAxis = -1;
                Bound *bo = o->start;
                while (bo != o->end->next) {
                    float bmin[3] = {bo->min.x, bo->min.y, bo->min.z}; 
                    float bmax[3] = {bo->max.x, bo->max.y, bo->max.z};
                    bool added = true;
                    for (int d = 0; d < 3; d++) {
                        if (!(
                            ((bmin[d] >= min[d]) || (bmax[d] >= min[d])) &&
                            ((bmin[d] <= max[d]) || (bmax[d] <= max[d]))
                           )) {
                            added = false;
                            break;
                        }
                    }
                    if (added) {
                        appendToContainer(c, *bo); // Make copy of bound rather than appending pointer
                    }
                    bo = bo->next;
                }
                containerCube(c, {1.f, 1.f, 1.f});
            }
        }
    }
    return out;
}

