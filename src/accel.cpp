#include "accel.hpp"

#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <thread>
#include "shape.hpp"
#include "vec.hpp"
#include "ray.hpp"

const char *accelerators[5] = {"Divide objects evenly", "Surface area heuristic (SAH)", "Voxel Grid", "Bi-tree (Disables BVH)", "Nothing like Glassner/Octree (Disables BVH)"};

namespace {
    // Line generated with distinctColors.py
    Vec3 distinctColors[16] = {{0.545, 0.271, 0.075}, {0.098, 0.098, 0.439}, {0.000, 0.502, 0.000}, {0.741, 0.718, 0.420}, {0.690, 0.188, 0.376}, {1.000, 0.000, 0.000}, {1.000, 0.647, 0.000}, {1.000, 1.000, 0.000}, {0.486, 0.988, 0.000}, {0.000, 0.980, 0.604}, {0.000, 1.000, 1.000}, {0.000, 0.000, 1.000}, {1.000, 0.000, 1.000}, {0.392, 0.584, 0.929}, {0.933, 0.510, 0.933}, {0.902, 0.902, 0.980}};
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

int whichSideCentroid(float centroid, float mid) {
    return centroid <= mid ? 0 : 1;
}

constexpr inline bool left(int v) {
    return v == 1 || v == 4;
}
constexpr inline bool right(int v) {
    return v == 3 || v == 4;
}

void whichOctants(bool *octants, Vec3 bMin, Vec3 bMax, Vec3 midPoint) {
    float side[3] = {0, 0, 0};
    for (int i = 0; i < 3;  i++) {
        if (bMin(i) < midPoint(i) || bMax(i) < midPoint(i)) {
            side[i] += 1;
        }
        if (bMin(i) >= midPoint(i) || bMax(i) >= midPoint(i)) {
            side[i] += 3;
        }
    }
    octants[0] = left(side[0]) && left(side[1]) && left(side[2]);
    octants[1] = right(side[0]) && left(side[1]) && left(side[2]);
    octants[2] = left(side[0]) && right(side[1]) && left(side[2]);
    octants[3] = right(side[0]) && right(side[1]) && left(side[2]);
    octants[4] = left(side[0]) && left(side[1]) && right(side[2]);
    octants[5] = right(side[0]) && left(side[1]) && right(side[2]);
    octants[6] = left(side[0]) && right(side[1]) && right(side[2]);
    octants[7] = right(side[0]) && right(side[1]) && right(side[2]);
}

float aabbSA(Vec3 a, Vec3 b) {
    Vec3 d = b - a;
    return 2.f*(d.x*d.y + d.x*d.z + d.y*d.z);
}

// splitOctree: 


// splitBitree: A bit like octree, except splits are just halving a node.
// "GLASSNER: Space Subdivision for Fast Ray Tracing"
// Set a fixed max number of objects per node/AABB.
// Recursively subdivide scene until this number of objects is reached.
// Makes things very simple, all we do here is tell the caller to split halfway along a cycling axis.
// Ignores the bvh param.
int splitBitree(Container *o, float *split, Bound *, Bound *, int *splitAxis, bool, int lastAxis, int maxNodesPerVox) {
    if (maxNodesPerVox < 1) return 1;
    if (o->size <= maxNodesPerVox) return 1;

    int axis = lastAxis+1;
    if (axis > 2) axis = 0;
    *splitAxis = axis;
    *split = o->max(axis) + 0.5f*(o->max(axis) - o->min(axis));
    // *split = o->centroid(axis);
    return 0;
    /*
    int shapeCount[2] = {0, 0};
    Bound *bo = o->start;
    while (bo != o->end->next) {
        int side = whichSide(bo->min(axis), bo->max(axis), o->a(axis), *split, o->b(axis));
        if (side == 0 || side == 2) {
            shapeCount[0]++;
        }
        if (side == 1 || side == 2) {
            shapeCount[1]++;
        }
        bo = bo->next;
    }*/
}

namespace {
    // Assumed cost values (C_o)
    // Assumed added cost of traversing the extra nodes added when splitting (C_i)
    const float cTraverse = 1.f;
    const float cSphere = 1.f;
    const float cAAB = 1.f;
}

void sahAxisSplits(int i, Container *o, float *spl, int *bestIndex, float *bestCost, Bound *bestBound, bool bvh, int *shapeCount, float costTriSphereRatio) {
    float cTri = costTriSphereRatio;

    int splitIndex = 0;
    bool countComplete = false;
    Bound *bsplit = o->start;
    while (bsplit != o->end->next) {
        Vec3 split = bvh ? bsplit->centroid : bsplit->max;
        *spl = split(i);
        // index 0 = spheres, 1 = triangles
        // float sa0[2] = {0.f, 0.f}, sa1[2] = {0.f, 0.f};
        Bound splitBounds[2];
        std::memset(&splitBounds, 0, sizeof(Bound)*2);
        splitBounds[0].min = {1e30f, 1e30f, 1e30f};
        splitBounds[0].max = {-1e30f, -1e30f, -1e30f};
        std::memcpy(&(splitBounds[1]), &(splitBounds[0]), sizeof(Bound));
        // int h = 0;
        int h0[3] = {0, 0, 0}, h1[3] = {0, 0, 0};
        Bound *bo = o->start;
        while (bo != o->end->next) {
            int shapeIndex = bo->s->type();
            if (!countComplete && shapeCount != NULL) {
                shapeCount[shapeIndex]++;
            }
            int side = -1;
            if (bvh) {
                side = whichSideCentroid(bo->centroid(i), *spl);
            } else {
                side = whichSide(bo->min(i), bo->max(i), o->min(i), *spl, o->max(i));
            }
            if (side == 0 || side == 2) {
                splitBounds[0].grow(bo);
                // sa0[shapeIndex] += surfaceAreas[j];
                h0[shapeIndex]++;
            }
            if (side == 1 || side == 2) {
                splitBounds[1].grow(bo);
                // sa1[shapeIndex] += surfaceAreas[j];
                h1[shapeIndex]++;
            }
            bo = bo->next;
        }
        /* for (int j = 0; j < 2; j++) {
            std::printf("bound[%d] = (%f %f %f - %f %f %f)\n", j, potentialSplit[j].min.x, potentialSplit[j].min.y, potentialSplit[j].min.z, potentialSplit[j].max.x, potentialSplit[j].max.y, potentialSplit[j].max.z);
        } */ 
        countComplete = true;
        // saCalculated = true;
        
        // We don't need to divide by totalSA, since its the same for all compared values
        
        float sa[2] = {aabbSA(splitBounds[0].min, splitBounds[0].max), aabbSA(splitBounds[1].min, splitBounds[1].max)};
        if (h0[0] + h0[1] == 0) {
            sa[0] = 0.f;
        }
        if (h1[0] + h1[1] == 0) {
            sa[1] = 0.f;
        }

        float cost = cTraverse + (sa[0]*(h0[ShapeType::Sphere]*cSphere + h0[ShapeType::Triangle]*cTri + h0[ShapeType::AAB]*cAAB)) + (sa[1]*(h1[ShapeType::Sphere]*cSphere + h1[ShapeType::Triangle]*cTri + h1[ShapeType::AAB]*cAAB));
        if (cost < *bestCost) {
            *bestIndex = splitIndex;
            *bestCost = cost;
            bestBound[0] = splitBounds[0];
            bestBound[1] = splitBounds[1];
        }
        splitIndex++;
        bsplit = bsplit->next;
    }
}

// splitSAH: Use the SAH heuristic:
// "MACDONALD J. D., BOOTH K. S.: Heuristics for Ray Tracing Using Space Subdivision."
// As discovered in review paper:
// "M. HAPALA, V. HAVRAN: Review: Kd-tree Traversal Algorithms for Ray Tracing"
// Based on the idea that rays intersecting an object is ~proportional to it's surface area.
// In it's current state, this is roughly O(3n^2).
int splitSAH(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int, float costTriSphereRatio) {
    // NOTES
    // Cost function given in paper IS suitable for now, as we do not yet cache intersections per ray (i.e. if leaf is in two AABBs, we currently test it twice).
    // Also see notes in sah.md
   
    const float cTri = costTriSphereRatio;

    Vec3 spl;

    int bestIndices[3] = {0, 0, 0};
    float bestCosts[3] = {1e30f, 1e30f, 1e30f};
    Bound bestBounds[3][2];

    // float *surfaceAreas = (float*)malloc(sizeof(float)*o->size);
    // shapeCount[0]: number of spheres, [1]: number of tris, [2]: number of aabs.
    int shapeCount[3] = {0, 0, 0};

    std::thread *axisThreads = new std::thread[3];

    for (int i = 0; i < 3; i++) {
        axisThreads[i] = std::thread(&sahAxisSplits, i, o, &(spl(i)), &(bestIndices[i]), &(bestCosts[i]), &(bestBounds[i][0]), bvh, i == 0 ? &shapeCount[0] : NULL, costTriSphereRatio);
        // sahAxisSplits(i, o, &(spl(i)), &(bestIndices[i]), &(bestCosts[i]), &(bestBounds[i][0]), bvh, i == 0 ? &shapeCount[0] : NULL);
        /*axisThreads[i] = std::thread([&, i]() {
        });
        */
    }

    for (int i = 0; i < 3; i++) {
        axisThreads[i].join();
    }

    delete[] axisThreads;
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
            *b0 = bestBounds[i][0];
            *b1 = bestBounds[i][1];
        }
    }

    // Determine the cost of just not traversing the given node unsplit, and check if it's any better.
    // float noSplitCost = (cSphere*shapeCount[0]*totalSA[0]) + (cTri*shapeCount[1]*totalSA[1]);
    float noSplitCost = aabbSA(o->min, o->max)*(cSphere*shapeCount[ShapeType::Sphere] + cTri*shapeCount[ShapeType::Triangle] + cAAB*shapeCount[ShapeType::AAB]);
    if (bestCost >= noSplitCost) {
        return 1;
    }
    *splitAxis = bestAxis;

    Bound *splitOn = o->at(bestIndices[bestAxis]);
    *split = (bvh ? splitOn->centroid : splitOn->max)(bestAxis);
    return 0;
}

// splitEqually: Heuristic find split that best divides the -number- of elements between the two children.
// sets splitAxis to the axis split on, and returns the float value of where that occurs on that axis.
int splitEqually(Container *o, float *split, Bound *b0, Bound *b1, int *splitAxis, bool bvh, int lastAxis, int) {
    Vec3 spl;

    int bestIndices[3] = {0, 0, 0};
    int bestSizes[3] = {9999, 9999, 9999};
    float bestRatios[3] = {-1e30, -1e30, -1e30};
    Bound bestBounds[3][2];

    for (int i = 0; i < 3; i++) {
        int splitIndex = 0;
        Bound *bsplit = o->start;
        while (bsplit != o->end->next) {
            spl(i) = bsplit->max(i);
            
            Bound splitBounds[2];
            std::memset(&splitBounds, 0, sizeof(Bound)*2);
            splitBounds[0] = Bound::forGrowing();
            splitBounds[1] = Bound::forGrowing();

            int h0 = 0, h1 = 0;
            Bound *bo = o->start;
            while (bo != o->end->next) {
                int side = -1;
                if (bvh) {
                    side = whichSideCentroid(bo->centroid(i), spl(i));
                } else {
                    side = whichSide(bo->min(i), bo->max(i), o->min(i), spl(i), o->max(i));
                }
                if (side == 0 || side == 2) {
                    splitBounds[0].grow(bo);
                    h0++;
                }
                if (side == 1 || side == 2) {
                    splitBounds[1].grow(bo);
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
                bestBounds[i][0] = splitBounds[0];
                bestBounds[i][1] = splitBounds[1];
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
            *b0 = bestBounds[i][0];
            *b1 = bestBounds[i][1];
        }
    }

    *splitAxis = bestAxis;

    Bound *splitOn = o->at(bestIndices[bestAxis]);
    *split = (bvh ? splitOn->centroid : splitOn->max)(bestAxis);
    
    return 0;
}

Container* generateOctreeHierarchy(Container *o, int splitLimit, int splitCount, int colorIndex, int maxNodesPerVox, int parentId) {
    if (splitCount >= splitLimit || o->size <= maxNodesPerVox || maxNodesPerVox < 1) return NULL;
    Container *out = new Container();
    out->min = o->min; // {1e30, 1e30, 1e30};
    out->max = o->max; // {-1e30, -1e30, -1e30};

    Vec3 voxDim = (o->max - o->min) / 2.f;

    Bound b[8];
    
    // x = l/r, y = u/d
    // z=0          z=1
    // -----        -----
    // |3|4|        |7|8|
    // -----        -----
    // |1|2|        |5|6|
    // -----        -----
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                int i = (z * 4) + (y * 2) + x;
                b[i].next = NULL;
                b[i].min = {o->min.x + (x*(voxDim.x)), o->min.y + (y*(voxDim.y)), o->min.z + (z*(voxDim.z))};
                b[i].max = b[i].min + voxDim;
                Container *c = new Container();
                c->min = b[i].min;
                c->max = b[i].max;
                c->id = (parentId*10)+i+1;
                b[i].s = c;
            }
        }
    }

    Vec3 midPoint = o->min + voxDim;

    Bound *bo = o->start;
    bool octantsOccupied[8];
    while (bo != o->end->next) {
        bool added = false;
        whichOctants((bool*)&octantsOccupied, bo->min, bo->max, midPoint);
        // std::printf("octantMap: ");
        for (int i = 0; i < 8; i++) {
            // std::printf("%d ", octantsOccupied[i]);
            if (octantsOccupied[i] == 0) continue;
            // Make sure the voxel isn't fully contained within the object
            // i.e. the voxel contains a face of the object.
            if (!bo->s->envelops(b[i])) {
                (dynamic_cast<Container*>(b[i].s))->append(*bo);
                added = true;
            }
        }
        // std::printf("\n");
        if (!added) std::printf("skipped!\n");
        bo = bo->next;
    }

    for (int i = 0; i < 8; i++) {
        Container *c = dynamic_cast<Container*>(b[i].s);
        if (c->size == 0) {
            delete b[i].s;
            continue;
        }

        Container *splitAgain = generateOctreeHierarchy(c, splitLimit, splitCount+1, colorIndex, maxNodesPerVox);
        colorIndex += (splitLimit - splitCount)*8;
        if (splitAgain != NULL) {
            c->clear(false);
            delete c;
            c = splitAgain;
            b[i].s = c;
        }
        // DEBUG SPHERES
        // containerSphereCorners(c);
        containerCube(c, distinctColors[colorIndex % 16]);
        colorIndex++;
      
        out->append(b[i]);
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


Container* generateHierarchy(Container *o, int accel, bool bvh, int splitLimit, int splitCount, int lastAxis, int colorIndex, int extra, float fextra) {
    if (splitCount >= splitLimit) return NULL;
    Container *out = new Container();
    out->min = o->min; // {1e30, 1e30, 1e30};
    out->max = o->max; // {-1e30, -1e30, -1e30};

    int bestAxis = 0;

    float splA = -1.f;
    Bound b[2];

    int stopSplitting = 0;
    if (accel == Accel::DivideObjectsEqually) {
        stopSplitting = splitEqually(o, &splA, &(b[0]), &(b[1]), &bestAxis, bvh, lastAxis, extra);
    } else if (accel == Accel::SAH) {
        stopSplitting = splitSAH(o, &splA, &(b[0]), &(b[1]), &bestAxis, bvh, lastAxis, fextra);
    } else if (accel == Accel::BiTree) {
        stopSplitting = splitBitree(o, &splA, &(b[0]), &(b[1]), &bestAxis, bvh, lastAxis, extra);
    }

    if (stopSplitting == 1) {
        std::printf("stopped splitting @ %d\n", splitCount);
        delete out;
        return NULL;
    }
    
    Container *containers[2] = {new Container(), new Container()};

    if (accel == Accel::BiTree) {
        for (int i = 0; i < 2;  i++) {
            Container *c = containers[i];
            /*if (c->size == 1) {
                std::memcpy(sections[i], c->start, sizeof(Shape));
                delete c;

            } else */
            c->min = out->min;
            c->max = out->max;
            c->splitAxis = bestAxis;

            Vec3 *boundary = &(c->max);
            if (i == 1) boundary = &(c->min);

            boundary->idx(bestAxis) = splA;
        }
    }

    Bound *bo = o->start;
    while (bo != o->end->next) {
        bool added = false;
        int side = -1;
        if (bvh) {
            side = whichSideCentroid(bo->centroid(bestAxis), splA);
        } else {
            side = whichSide(bo->min(bestAxis), bo->max(bestAxis), o->min(bestAxis), splA, o->max(bestAxis));
        }
        if (side == 0 || side == 2) {
            // For an octree, check if any of the faces (estimated) are actually in the node.
            // A tri has no volume, hence one cannot be "inside" and not on a surface. Therefore we can assume a face is found within the node.
            // A sphere is a different case, however.
            bool fullyContained = (accel == Accel::BiTree) ? b->s->envelops(containers[0]->min, containers[0]->max) : false;
            if (!fullyContained) {
                containers[0]->append(*bo); // Make copy rather than appending existing pointer
                added = true;
            }
            /* if (bvh) {
                bbEdges[0] = std::max(bbEdges[0], bo->max(bestAxis));
            } */
        }
        if (side == 1 || side == 2) {
            bool fullyContained = (accel == Accel::BiTree) ? b->s->envelops(containers[1]->min, containers[1]->max) : false;
            if (!fullyContained) {
                containers[1]->append(*bo); // Make copy rather than appending existing pointer
                added = true;
            }
            /* if (bvh) {
                bbEdges[1] = std::min(bbEdges[1], bo->min(bestAxis));
            } */
        }
        if (!added) std::printf("skipped!\n");
        bo = bo->next;
    }
   
    for (int i = 0; i < 2;  i++) {
        Container *c = containers[i];
        /*if (c->size == 1) {
            std::memcpy(sections[i], c->start, sizeof(Shape));
            delete c;

        } else */
        if (c->size != 0) {
            if (bvh) {
                c->min = b[i].min;
                c->max = b[i].max;
            } else {
                c->min = out->min;
                c->max = out->max;
                c->splitAxis = bestAxis;

                Vec3 *boundary = &(c->max);
                if (i == 1) boundary = &(c->min);

                boundary->idx(bestAxis) = splA;
            }
            Container *splitAgain = generateHierarchy(c, accel, bvh, splitLimit, splitCount+1, bestAxis, colorIndex, extra, fextra);
            colorIndex += (splitLimit - splitCount)*2;
            if (splitAgain != NULL) {
                delete c;
                c = splitAgain;
            }
            // DEBUG SPHERES
            // containerSphereCorners(c);
            containerCube(c, distinctColors[colorIndex % 16]);
            colorIndex++;
            
            b[i].min = c->min;
            b[i].max = c->max;
            b[i].s = c;
            out->append(b[i]);
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
        Container *subContainer = dynamic_cast<Container*>(current);
        if (subContainer != nullptr) {
            char splitAxis = 'x';
            if (subContainer->splitAxis == 1) splitAxis = 'y';
            else if (subContainer->splitAxis == 2) splitAxis = 'z';
            std::printf("%s%snode(%d%c, (%.3f, %.3f %.3f), (%.3f, %.3f, %.3f)): {%s\n",
                    prefix.c_str(), C_BLUE, subContainer->size, splitAxis,
                    subContainer->min.x, subContainer->min.y, subContainer->min.z,
                    subContainer->max.x, subContainer->max.y, subContainer->max.z,
                    C_RESET);
            printShapes(subContainer, tabIndex+1);
            std::printf("%s%s}%s;\n", prefix.c_str(), C_BLUE, C_RESET);
        } else {
            std::printf("%s%sleaf%s ", prefix.c_str(), C_GREEN, C_RESET);
            std::cout << current->name() << " ";
            std::printf("color(%.3f, %.3f, %.3f);\n", current->mat()->color.x, current->mat()->color.y, current->mat()->color.z);
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
    float pts[2][3] = {{c->min.x, c->min.y, c->min.z}, {c->max.x, c->max.y, c->max.z}};
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
            Triangle *t0 = new Triangle();
            t0->oA = Vec3{corners[0][0], corners[0][1], corners[0][2]};
            t0->oB = Vec3{corners[1][0], corners[1][1], corners[1][2]};
            t0->oC = Vec3{corners[2][0], corners[2][1], corners[2][2]};
            t0->debug = true;
            t0->material = new Material();
            t0->mat()->color = color;
            t0->applyTransform();
            c->append(t0);
            Triangle *t1 = new Triangle();
            t1->oA = Vec3{corners[2][0], corners[2][1], corners[2][2]};
            t1->oB = Vec3{corners[3][0], corners[3][1], corners[3][2]};
            t1->oC = Vec3{corners[0][0], corners[0][1], corners[0][2]};
            t1->debug = true;
            t1->material = new Material();
            t1->mat()->color = color;
            t1->applyTransform();
            c->append(t1);
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
        Sphere *s = new Sphere();
        s->center = (j == 0) ? c->min : c->max;
        s->radius = 0.3f;
        s->thickness = 1.f;
        s->material = new Material();
        s->mat()->color = color;
        s->mat()->opacity = 1.f;
        s->mat()->reflectiveness = 0.f;
        s->mat()->specular = 0.f;
        s->mat()->shininess = 5.f;
        bo.s = s;
        c->append(bo);
    }
}

void getVoxelIndex(Container *c, int subdivision, Vec3 p, Vec3 delta, int *x, int *y, int *z, float *t) {
    Vec3 dims = c->max - c->min;
    Vec3 vox = dims / subdivision;
    // std::printf("got vox(%f %f %f\n", vox.x, vox.y, vox.z);
    Vec3 ap = (p - c->min);
    bool originWithinVoxel = true;
    for (int i = 0; i < 3; i++) {
        if (ap(i) < 0 || ap(i) > dims(i)) {
            originWithinVoxel = false;
            break;
        }
    }
    if (originWithinVoxel) {
        Vec3 apVox = {ap.x/vox.x, ap.y/vox.y, ap.z/vox.z};
        *x = apVox.x;
        *y = apVox.y;
        *z = apVox.z;
        *t = 0;
        return;
    }
    // If our origin "p" isn't in a voxel already (i.e. out the scene), find the point along the ray where it hits one.
    *t = meetAABB(ap, delta, {0.f, 0.f, 0.f}, dims);
    if (*t < -9990.f) {
        *x = -1;
        *y = -1;
        *z = -1;
        *t = -1.f;
        return;
    }
    ap = ap + (*t * delta);
    Vec3 apVox;
    for (int i = 0; i < 3; i++) {
        apVox(i) = ap(i)/vox(i);
    }
   
    /* Cubes are back-to-back, and so in grid-coordinates,
     * one might span the space 0-0.99999 and their neighor 1-1.99999.
     * If we look at the back (i.e. the maximum edge) of a cube on
     * the edge of our grid, the coordinate will land on a clean .0 value,
     * which is actually out of bounds.
     * Therefore, we just cap the integer-ized coordinates at (subdivision-1). */
    *x = apVox.x;
    if (*x == subdivision) *x -=1;
    *y = apVox.y;
    if (*y == subdivision) *y -=1;
    *z = apVox.z;
    if (*z == subdivision) *z -=1;
}

// Split container into subcontainer "voxel" grid of dimensions subdivision^3.
// "start"/"end" is a pointer to start of a subdivision^3 array.
Container* splitVoxels(Container *o, int subdivision) {
    Container *out = new Container();
    out->min = {1e30, 1e30, 1e30};
    out->max = {-1e30, -1e30, -1e30};
    out->voxelSubdiv = subdivision;
    out->min = o->min;
    out->max = o->max;
    Vec3 dims = o->max - o->min;
    Vec3 vox = dims / subdivision;
    std::printf("global dim(%f %f %f), vox(%f %f %f)\n", dims.x, dims.y, dims.z, vox.x, vox.y, vox.z);
    size_t totalSize = subdivision*subdivision*subdivision;
    out->size = totalSize;
    Bound *grid = new Bound[totalSize];
    std::memset(grid, 0, sizeof(Bound)*totalSize);
    out->append(grid);
    for (int z = 0; z < subdivision; z++) {
        for (int y = 0; y < subdivision; y++) { 
            for (int x = 0; x < subdivision; x++) { 
                Bound *b = grid + x + subdivision * (y + subdivision * z);
                Container *c = new Container();
                b->s = c;
                c->min = o->min + Vec3{vox.x * x, vox.y * y, vox.z * z};
                c->max = c->min + vox;
                c->bounds(b);
                c->splitAxis = -1;
                Bound *bo = o->start;
                while (bo != o->end->next) {
                    bool added = true;
                    for (int d = 0; d < 3; d++) {
                        if (!(
                            ((bo->min(d) >= c->min(d)) || (bo->max(d) >= c->min(d))) &&
                            ((bo->min(d) <= c->max(d)) || (bo->max(d) <= c->max(d)))
                           )) {
                            added = false;
                            break;
                        }
                    }
                    if (added) {
                        c->append(*bo); // Make copy of bound rather than appending pointer
                    }
                    bo = bo->next;
                }
                containerCube(c, {1.f, 1.f, 1.f});
            }
        }
    }
    return out;
}

