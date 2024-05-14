#include "map.hpp"

#include "shape.hpp"
#include "util.hpp"
#include "ray.hpp"
#include "mat.hpp"
#include "accel.hpp"
#include <cmath>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <thread>

#define RI_AIR 1.f
#define RI_GLASS 1.52f

const char *modes[3] = {"Raycasting", "Raycasting w/ Reflections", "Lighting w/ Phong Specular"};

double WorldMap::castRays(Image *img, RenderConfig *rc, double (*getTime)(void), int nthreads) {
    if (nthreads == -1) nthreads = std::thread::hardware_concurrency();
    currentlyRendering = true;
    if (rc->baseBrightness == -1.f) {
        rc->baseBrightness = baseBrightness;
    }
    if (rc->baseBrightness != baseBrightness) {
        baseBrightness = rc->baseBrightness;
    }
    if (rc->globalShininess == -1.f) {
        rc->globalShininess = globalShininess;
    }
    if (rc->globalShininess != globalShininess) {
        globalShininess = rc->globalShininess;
    }
    if (rc->manualPosition != cam->position) {
        cam->setPosition(rc->manualPosition);
    }
    // Image is split into horizontal strips for each thread (Previously vertical, because i forgot about contiguous memory access)
    // int sectionWidth = cam->w / nthreads;
    int sectionHeight = cam->h / nthreads;
    std::thread *threadPtrs = new std::thread[nthreads];
    rc->threadStates = new int[nthreads];
    rc->nthreads = nthreads;
    lastRenderTime = getTime();
    // std::printf("Thread line height: [1..n-1]:%d, [n]:%d\n", int(sectionHeight), int(sectionHeight) + (cam->h % nthreads));
    for (int i = 0; i < nthreads; i++) {
        // int w = sectionWidth;
        int h = sectionHeight;
        // int w0 = i*w;
        int h0 = i*h;
        if (i == nthreads-1) {
            // w += (cam->w % nthreads);
            h += (cam->h % nthreads);
        }
        // int w1 = w0 + w;
        int h1 = h0 + h;
        // threadptrs[i] = std::thread(&WorldMap::castSubRays, this, img, rc, w0, w1, 0, cam->h);
        threadPtrs[i] = std::thread(&WorldMap::castSubRays, this, img, rc, 0, cam->w, h0, h1, &(rc->threadStates[i]));
    }
    for (int i = 0; i < nthreads; i++) {
        threadPtrs[i].join();
    }
    lastRenderTime = getTime() - lastRenderTime;
    currentlyRendering = false;
    delete[] threadPtrs;
    delete[] rc->threadStates;
    rc->threadStates = NULL;
    // free(res);
    return lastRenderTime;
}

void WorldMap::createSphere(Vec3 center, float radius, Vec3 color, float opacity, float reflectiveness, float specular, float shininess, float thickness) {
    Shape *sh = emptyShape();
    sh->s = (Sphere*)alloc(sizeof(Sphere));
    sh->s->center = center;
    sh->s->radius = radius;
    sh->s->thickness = thickness;
    sh->color = color;
    sh->opacity = opacity;
    sh->reflectiveness = reflectiveness;
    sh->specular = specular;
    sh->shininess = shininess;
    appendToContainer(&unoptimizedObj, sh);
}

void WorldMap::createTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float opacity, float reflectiveness, float specular, float shininess) {
    Shape *sh = emptyShape();
    sh->t = (Triangle*)alloc(sizeof(Triangle));
    sh->t->a = a;
    sh->t->b = b;
    sh->t->c = c;
    sh->color = color;
    sh->opacity = opacity;
    sh->reflectiveness = reflectiveness;
    sh->specular = specular;
    sh->shininess = shininess;
    appendToContainer(&unoptimizedObj, sh);
}

void WorldMap::castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount) {
    RayResult bounce = RayResult();
    castRay(&bounce, obj, p0, delta, rc, callCount);
    Vec3 color = {0.f, 0.f, 0.f};
    if (bounce.collisions > 0) {
        color = bounce.color;
    }
    res->reflectionColor = color;
}

void WorldMap::castShadowRays(Vec3 viewDelta, Vec3 p0, RenderConfig *rc, RayResult *res) {
    Vec3 lightColor = Vec3{1.f, 1.f, 1.f} * rc->baseBrightness;
    Vec3 specularColor = Vec3{0.f, 0.f, 0.f};
    RenderConfig simpleConfig;
    simpleConfig.collisionsOnly = true;
    simpleConfig.triangles = rc->triangles;
    simpleConfig.mtTriangleCollision = rc->mtTriangleCollision;
    simpleConfig.spheres = rc->spheres;
    simpleConfig.maxBounce = rc->maxBounce;
    simpleConfig.showDebugObjects = rc->showDebugObjects;
    RayResult r = RayResult();
    for (PointLight light: pointLights) {
        Vec3 distance = light.center - p0;
        float tLight = mag(distance);
        Vec3 normDistance = distance / tLight;
        r.resetObj();
        ray(&r, obj, p0, normDistance, &simpleConfig);
        bool noHit = r.collisions == 0 || r.t > tLight;
        if (!noHit) continue;
        float scaledDistance = tLight / rc->distanceDivisor;
        float lightBrightness = light.brightness / (scaledDistance*scaledDistance);
        lightColor = lightColor + (light.color * lightBrightness);

        if (!rc->specular) continue;
        // PHONG : CGPaP in C p.730 Sect. 16.1.4
        // \hat{L} = normDistance,
        // \hat{N} = norm(res->normal),
        Vec3 normNorm = norm(res->normal);
        // \hat{V} = norm(viewDelta)
        // \hat{R} = rNorm;
        Vec3 rNorm = norm((2.f * normNorm * dot(normNorm, normDistance)) - normDistance);
        // k_s = res->specular,
        // O_{s\varlambda} (Specular Colour) = light.specular * light.specularColor,
        // n (shininess):
        float n = (res->obj->shininess == -1.f) ? globalShininess : res->obj->shininess;
        float rDotV = std::fmax(std::pow(dot(rNorm, norm(viewDelta)), n), 0);
        // std::printf("got rDotV %f\n", rDotV);
        // std::printf("specular color %f %f %f\n", light.specularColor.x, light.specularColor.y, light.specularColor.z);
        specularColor = specularColor + (res->obj->specular * (light.specular * light.specularColor) * rDotV);
    }
    res->specularColor = specularColor;
    res->color = (res->color * lightColor);
}

void castThroughSolidSphere(float r0, float r1, Vec3 pb, Vec3 delta, Sphere *s, Vec3 *pOut, Vec3 *deltaOut) {
    Vec3 normal = pb - s->center;
    bool tir = true;
    Vec3 r = Refract(r0, r1, delta, normal, &tir);
    if (tir) {
        r = delta - 2.f*dot(delta, normal) * normal;
        *deltaOut = r;
        *pOut = pb + (0.001f * r);
        return;
    }
    tir = true;
    while (tir) {
        pb = pb + (0.001f * r);
        float t = meetsSphere(pb, r, s, NULL);
        pb = pb + (t * r);
        Vec3 n = -1.f*(pb - s->center);
        Vec3 r2 = Refract(r1, r0, r, n, &tir);
        if (tir) {
            r = r - 2.f*dot(r, n) * n;
        } else {
            r = r2;
        }
    }
    *pOut = pb;
    *deltaOut = r;
}

void WorldMap::castThroughSphere(Vec3 delta, RenderConfig *rc, RayResult *res, int callCount) {
    float ri = rc->refractiveIndex;
    float thickness = res->obj->s->thickness;
    Vec3 center = res->obj->s->center;
    if (thickness == 0.f) {
        // Behave like a filled sphere of air.
        ri = RI_AIR;
        thickness = 1.f;
    }

    Vec3 pb = res->p0;
    Vec3 refract;
    if (thickness == 1.f) {
        castThroughSolidSphere(RI_AIR, ri, res->p0, delta, res->obj->s, &pb, &refract);
    } else {
        // Into outer sphere (RI_AIR -> RI_SPHERE)
        Vec3 normal = pb - center;
        bool tir = true;
        Vec3 r = Refract(RI_AIR, ri, delta, normal, &tir);
        if (tir) {
            // Reflect off the sphere.
            refract = delta - 2.f*dot(delta, normal) * normal;
        } else {
            bool escaped = false;
            refract = r;
            // Our smaller, inner sphere, filled with RI_AIR.
            Sphere innerSphere = {center, res->obj->s->radius*(1.f - res->obj->s->thickness), 1.f};
            while (!escaped) {
                // Hit the inner sphere
                float t = meetsSphere(pb, refract, &innerSphere, NULL);
                if (t >= 0) {
                    pb = pb + (t * refract);
                    Vec3 p2;
                    // Refract in and out of the inner sphere (RI_SPHERE -> RI_AIR -> RI_SPHERE)
                    castThroughSolidSphere(ri, RI_AIR, pb, refract, &innerSphere, &p2, &r);
                    pb = p2;
                    refract = r;
                    // Hit the outer sphere
                    t = meetsSphere(pb, refract, res->obj->s, NULL);
                    pb = pb + (t * refract);
                    normal = -1.f * (pb - center);
                    // Out of the outer sphere (RI_SPHERE -> RI_AIR)
                    r = Refract(ri, RI_AIR, refract, normal, &tir);
                    if (tir) {
                        // If we have TIR, reflect back into the sphere, and loop this process again.
                        refract = refract - 2.f*dot(refract, normal) * normal;
                    } else {
                        // If we escape the outer sphere, the loop ends.
                        refract = r;
                        escaped = true;
                    }
                } else { // Missing the internal sphere means we can ignore its existence.
                    castThroughSolidSphere(RI_AIR, ri, res->p0, delta, res->obj->s, &pb, &refract);
                    escaped = true;
                }
            }
        }
    }
   
    // Finally, cast a ray out from the sphere.
    pb = pb + (0.001f * refract);
    RayResult r = RayResult();
    castRay(&r, obj, pb, refract, rc, callCount+1);
    res->refractColor = r.color;
}

// FIXME?:
// An object can span multiple voxels, so to avoid intersecting twice,
// tag an object with the ID of the ray on intersection.
// Then before intersecting an object, check if the object has been intersected by this rayID.
// HOWEVER, unless we used a map per-ray to store intersections or something, this wouldn't work with multiple threads.
void WorldMap::voxelRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc) {
    delta = norm(delta);
    int x = 0, y = 0, z = 0;
    int subdivision = optimizeLevel;
    Vec3 dims = c->b - c->a;
    Vec3 vox = dims / subdivision;
    float t = 0;
    // Find the first voxel our ray hits, store it in (x, y, z), and how far long the ray we've traveled to get there in "t".
    getVoxelIndex(c, subdivision, p0, delta, &x, &y, &z, &t);
   
    if (x < 0) return;

    // Set our start point to where we hit the first voxel,
    // and reset our "t" (ray distance) back to zero.
    p0 = p0 + (t * delta);
    t = 0.f;
    
    // Get the min/max corners of our home voxel.
    Vec3 cVoxMin = c->a + Vec3{float(x)*vox.x, float(y)*vox.y, float(z)*vox.z};
    Vec3 cVoxMax = cVoxMin + vox;

    // stepX/Y/Z, indicating the direction on each axis.
    int step[3] = {
        delta.x > 0.f ? 1 : (delta.x == 0.f ? 0 : -1),
        delta.y > 0.f ? 1 : (delta.y == 0.f ? 0 : -1),
        delta.z > 0.f ? 1 : (delta.z == 0.f ? 0 : -1)
    };

    // tMaxX/Y/Z: How far we need to travel to hit our home voxel's x/y/z boundaries.
    Vec3 tMax = {0,0,0};
    for (int i = 0; i < 3; i++) {
        if (step[i] == 0) {
            // Makes sure it's never the smallest, and so never gets incremented.
            tMax(i) = 1e10f;
        } else {
            float boundary = step[i] == 1 ? cVoxMax(i) : cVoxMin(i);
            tMax(i) = (boundary - p0(i)) / delta(i);
        }
    }

    // tDeltaX/Y/Z: How far to travel until we've gone a distance equal to the depth/height/width of a voxel.
    Vec3 tDelta = {0,0,0};
    for (int i = 0; i < 3; i++) {
        tDelta(i) = vox(i) / std::abs(delta(i));
    }


    // Get the bound containing the voxel at (x, y, z).
    Bound *o = c->start + x + subdivision * (y + subdivision * z);
    // Loop until:
    // we find an empty voxel,
    // we've hit a solid object* (see transparency caveat below)
    // we go out of the grid bounds.
    while (o != NULL) {
        // Check if the bound contains a non-empty container.
        bool empty = !(o->s != NULL && o->s->c != NULL && o->s->c->size > 0);
        if (!empty) {
            traversalRay(res, o->s->c, p0, delta, rc);
            // the caller, castRay, only casts additional transparency rays if we hit anything behind (i.e. res->collisions > 1), therefore we can only quit if we've hit something solid, or more than 1 object.
            if (res->obj != NULL && (res->obj->opacity == 1.f || res->collisions > 1)) {
                o = NULL;
                break;
            }
        }

        // traverse to the next voxel.
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                x += step[0];
                if (x < 0 || x >= subdivision) {
                    o = NULL;
                    break;
                }
                tMax.x += tDelta.x;
            } else {
                z += step[2];
                if (z < 0 || z >= subdivision) {
                    o = NULL;
                    break;
                }
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                y += step[1];
                if (y < 0 || y >= subdivision) {
                    o = NULL;
                    break;
                }
                tMax.y += tDelta.y;
            } else {
                z += step[2];
                if (z < 0 || z >= subdivision) {
                    o = NULL;
                    break;
                }
                tMax.z += tDelta.z;
            }
        }

        // Get the bound containing the voxel at (x, y, z).
        o = c->start + x + subdivision * (y + subdivision * z);
    }
}

void WorldMap::traversalRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc) {
    Bound *bo = c->start;
    // When using --bi-trees/kd-trees--, the two sub-nodes always span the area of the parent.
    // Missing one of them means we must hit the other.
    // Hitting one does not imply we dont't hit the other, so "true" can be our default state.
    bool siblingContainerCollision = true;
    while (bo != c->end->next) {
        Shape *current = bo->s;
        if (current->debug && !(rc->showDebugObjects)) {
            bo = bo->next;
            continue;
        }
        if (current->s != NULL && rc->spheres) {
            float t = meetsSphere(p0, delta, current->s, NULL);
            if (t >= 0) {
                res->collisions++;
                res->potentialCollisions++;
                if (t < res->t) {
                    res->obj = current;
                    res->t = t;
                    res->p0 = p0 + (res->t * delta);
                    res->normal = res->p0 - current->s->center;
                    res->norm = norm(res->normal);
                }
            }
        } else if (current->t != NULL && rc->triangles) {
            Vec3 normal = getVisibleTriNormal(delta, current->t->a, current->t->b, current->t->c);
            Vec3 nNormal = norm(normal);
            if (rc->mtTriangleCollision) {
                float t = meetsTriangleMT(p0, delta, current->t);
                if (t >= 0 && t < res->t) {
                    res->potentialCollisions++;
                    res->collisions++;
                    res->obj = current;
                    res->t = t;
                    res->p0 = p0 + (t *delta);
                    res->normal = normal;
                    res->norm = nNormal;
                }
            } else {
                float t = meetsTrianglePlane(p0, delta, nNormal, current->t);
                if (t >= 0) {
                    res->potentialCollisions++;
                    if (t < res->t) {
                        Vec3 cPoint = p0 + (t * delta);
                        if (meetsTriangle(nNormal, cPoint, current->t)) {
                            res->collisions++;
                            res->obj = current;
                            res->t = t;
                            res->p0 = cPoint;
                            res->normal = normal;
                            res->norm = nNormal;
                        } else {
                            res->potentialCollisions--;
                        }
                    }
                }
            }
        } else if (current->c != NULL) {
            bool collision = false;
            if (!siblingContainerCollision && accelIndex == Accel::BiTree) {
                // Bi-tree, see comment at top of this method
                collision = true;
            } else if (current->c->plane) {
                // Test container collision
                Triangle tris[2] = {
                    {current->c->a, current->c->b, current->c->c},
                    {current->c->c, current->c->d, current->c->a},
                };
                collision = !(rc->planeOptimisation);
                if (!collision) {
                    for (int i = 0; i < 2; i++) {
                        Vec3 normal = norm(getVisibleTriNormal(delta, tris[i].a, tris[i].b, tris[i].c));
                        float t = meetsTrianglePlane(p0, delta, normal, &(tris[i]));
                        if (t < 0 || t >= res->t) continue;
                        Vec3 collisionPoint = p0 + (t * delta);
                        if (meetsTriangle(normal, collisionPoint, &(tris[i]))) {
                            collision = true;
                            break;
                        }
                    }
                }
            } else {
                collision = meetsAABB(p0, delta, current->c);
                if (!collision) {
                    siblingContainerCollision = false;
                }
                // collision = true;
            }
            // If collision, cast ray to objects within the container
            if (collision) {
                ray(res, current->c, p0, delta, rc);
            }
        }
        bo = bo->next;
    }
}

void WorldMap::ray(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc) {
    if (accelIndex == Accel::Voxel && obj == optimizedObj) {
        voxelRay(res, c, p0, delta, rc);
    } else {
        traversalRay(res, c, p0, delta, rc);
    }
}

// Passed res MUST be initialized with the rayResult constructor, or wiped with resetObj()!
void WorldMap::castRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount) {
    if (callCount > rc->maxBounce) {
        // std::printf("Terminating!\n");
        return;
    }

    // Collision Detection
    ray(res, c, p0, delta, rc);

    if (res->obj == NULL) return;

    // Shading
    res->color = res->obj->color;

    if (res->obj->s != NULL && res->obj->texId != -1) {
        Vec2 uv = sphereUV(res->obj->s, res->p0);
        // res->color = Vec3{uv.x, uv.y, 0.5f};
        Texture *tx = tex.at(res->obj->texId);
        if (tx != NULL) res->color = tx->at(uv.x, uv.y);
    }

    // std::printf("looped over %d objects\n", size);
    if (rc->collisionsOnly) return;
    
    // FIXME: This shouldn't be necessary, but without it, bouncing rays collide with the sphere they bounce off, causing a weird moiré pattern. To avoid, this moves the origin just further than the edge of the sphere.
    Vec3 p0PlusABit = res->p0 + (0.01f * res->norm);
    if (rc->reflections && res->obj->reflectiveness != 0) {
        // Angle of reflection: \vec{d} - 2(\vec{d} \cdot \vec{n})\vec{n}
        Vec3 reflection = delta - 2.f*dot(delta, res->norm) * res->norm;
        castReflectionRay(p0PlusABit, reflection, rc, res, callCount+1);
        res->color = ((1.f - res->obj->reflectiveness)*res->color);
    }

    if (rc->lighting) {//  && res->obj->reflectiveness != 0) {
        castShadowRays(-1.f*delta, p0PlusABit, rc, res);
    }

    if (res->obj->opacity != 1.f) {
        // FIXME: Make this more efficient. Maybe don't cast another ray, and just store all collisions data?
        if (res->collisions >= 1 && res->potentialCollisions > 1) {
            Vec3 p0Transparent = res->p0 - (0.01f * res->norm);
            if (res->obj->t != NULL) {
                RayResult r = RayResult();
                castRay(&r, obj, p0Transparent, delta, rc, callCount+1);
                res->refractColor = r.color;
            } else if (res->obj->s != NULL) {
                castThroughSphere(delta, rc, res, callCount);
            }
        } else {
            res->refractColor = {0.f, 0.f, 0.f};
        }
    }
   
    res->color = (res->obj->opacity * res->color) + ((1.f - res->obj->opacity) * res->refractColor) + (res->obj->reflectiveness * res->reflectionColor) + res->specularColor;

    res->color.x = std::fmin(res->color.x, 1.f);
    res->color.y = std::fmin(res->color.y, 1.f);
    res->color.z = std::fmin(res->color.z, 1.f);
}

void WorldMap::castSubRays(Image *img, RenderConfig *rc, int w0, int w1, int h0, int h1, int *state) {
    RayResult res = RayResult();
    Vec3 startCol = w0 * cam->viewportCol;
    Vec3 startRow = h0 * cam->viewportRow;
    Vec3 baseRowVec = cam->viewportCorner + startRow + startCol;
    *state = 1;
    for (int y = h0; y < h1; y++) {
        // cam->viewportCorner is a vector from the origin, therefore all calculated pixel positions are.
        Vec3 delta = baseRowVec;
        for (int x = w0; x < w1; x++) {
            res.resetObj();
            castRay(&res, obj, cam->position, delta, rc);
            if (res.collisions > 0) {
                writePixel(img, x, y, res.color);
            }
            
            delta = delta + cam->viewportCol; 
            if (*state == -1) break;
        }
        baseRowVec = baseRowVec + cam->viewportRow;
        if (*state == -1) break;
    }
    *state = 0;
}

namespace {
    const char* w_map = "map";
    const char* w_dimensions = "dimensions";
    const char* w_brightness = "brightness";
    const char* w_sphere = "sphere";
    const char* w_triangle = "triangle";
    const char* w_pointLight = "plight";
    const char* w_shininess = "shininess";
    const char* w_include = "include";
    const char* w_translate = "translate";
    const char* w_rotate = "rotate";
    const char* w_scale = "scale";
    const char* w_comment = "//";
    const char* w_container = "container";
    const char* w_campreset = "campreset";
    const char* w_a = "a";
    const char* w_b = "b";
    const char* w_c = "c";
    const char* w_d = "d";
    const char* w_open = "{";
    const char* w_close = "}";
}

void WorldMap::encode(char const* path) {
    std::ofstream out(path);
    if (!path) {
        throw std::runtime_error("Failed to open " + std::string(path));
        return;
    }
    {
        // FIXME: camera maybe?
    }
    {
        // World dimensions
        std::ostringstream fmt;
        fmt << w_map << " " << w_dimensions << " " << w << " " << h << " " << d << " ";
        fmt << w_brightness << " " << baseBrightness << " ";
        fmt << w_shininess << " " << globalShininess << " ";
        fmt << std::endl;
        out << fmt.str();
    }
    Bound *bo = unoptimizedObj.start;
    while (bo != unoptimizedObj.end) {
        Shape *current = bo->s;
        if (current->s != NULL) out << encodeSphere(current);
        else if (current->t != NULL) out << encodeTriangle(current);
        bo = bo->next;
    }

    out.close();
    std::printf("Current map saved to %s\n", path);
}

WorldMap::WorldMap(char const* path) {
    optimizeLevel = 0;
    accelIndex = Accel::None;
    currentlyRendering = false;
    currentlyOptimizing = false;
    lastRenderTime = 0.f;
    obj = &unoptimizedObj;
    std::memset(&unoptimizedObj, 0, sizeof(Container));
    clearContainer(&unoptimizedObj, true);
    flatObj = NULL;
    clearContainer(flatObj, true);
    optimizedObj = NULL;
    camPresetNames = NULL;
    loadFile(path);
}

void WorldMap::optimizeMap(double (*getTime)(void), int level, int accelIdx) {
    currentlyOptimizing = true;
    optimizeLevel = level;
    accelIndex = accelIdx;
    std::printf("Optimizing map with accelerator %d, depth %d, extra params %d, %f\n", accelIndex, level, accelParam, accelFloatParam);
    clearContainer(optimizedObj, false);
    free(optimizedObj);
    optimizedObj = NULL;
    if (flatObj == NULL) {
        flatObj = emptyContainer();
        flattenRootContainer(flatObj, &unoptimizedObj);
    }
    lastOptimizeTime = getTime();
    if (accelIndex == Accel::Voxel) {
        optimizedObj = splitVoxels(flatObj, level);
    } else if (accelIndex == Accel::BiTree) {
        optimizedObj = generateHierarchy(flatObj, accelIndex, false, level, 0, -1, 0, accelParam);
    } else if (accelIndex == Accel::FalseOctree) {
        optimizedObj = generateOctreeHierarchy(flatObj, level, 0, 0, accelParam);
    } else {
        optimizedObj = generateHierarchy(flatObj, accelIndex, bvh, level, 0, -1, 0, accelParam, accelFloatParam);
    }
    lastOptimizeTime = getTime() - lastOptimizeTime;
    currentlyOptimizing = false;
}

void WorldMap::loadFile(char const* path) {
    std::printf("Frees: %d\n", clearContainer(&unoptimizedObj, true));
    pointLights.clear();
    camPresets.clear();
    free(camPresetNames);
    clearContainer(optimizedObj, false);
    free(optimizedObj);
    optimizedObj = NULL;
    clearContainer(flatObj, true);
    flatObj = NULL;
    obj = &unoptimizedObj;

    mapStats.spheres = 0, mapStats.tris = 0, mapStats.lights = 0;
    mapStats.name.clear();
    mapStats.name = std::string(path);

    tex.clear();

    std::printf("Allocations: %d\n", loadObjFile(path));
    camPresetNames = (const char**)malloc(sizeof(char*)*camPresets.size());
    for (size_t i = 0; i < camPresets.size(); i++) {
        camPresetNames[i] = camPresets.at(i).name.c_str();
    }
}

int WorldMap::loadObjFile(const char* path, Mat4 transform) {
    std::ifstream in(path);
    std::string line;
    Container *c = NULL;
    int allocCounter = 0;
    while (std::getline(in, line)) {
        std::stringstream lstream(line);
        std::string token;
        lstream >> token;
        if (token == w_map) {
            lstream >> token;
            if (token == w_dimensions) {
                lstream >> token;
                w = std::stof(token);
                lstream >> token;
                h = std::stof(token);
                lstream >> token;
                d = std::stof(token);
            }
            lstream >> token;
            if (token == w_brightness) {
                lstream >> token;
                baseBrightness = std::stof(token);
            } else {
                lstream << " " << token;
            }
            lstream >> token;
            if (token == w_shininess) {
                lstream >> token;
                globalShininess = std::stof(token);
            } else {
                globalShininess = 1.f;
                lstream << " " << token;
            }
        } else if (token == w_container) {
            c = emptyContainer(true);
            allocCounter++;
            for (int i = 0; i < 4; i++) {
                lstream >> token;
                if (token == w_a) {
                    lstream >> token;
                    c->a.x = std::stof(token);
                    lstream >> token;
                    c->a.y = std::stof(token);
                    lstream >> token;
                    c->a.z = std::stof(token);
                } else if (token == w_b) {
                    lstream >> token;
                    c->b.x = std::stof(token);
                    lstream >> token;
                    c->b.y = std::stof(token);
                    lstream >> token;
                    c->b.z = std::stof(token);
                } else if (token == w_c) {
                    lstream >> token;
                    c->c.x = std::stof(token);
                    lstream >> token;
                    c->c.y = std::stof(token);
                    lstream >> token;
                    c->c.z = std::stof(token);
                } else if (token == w_d) {
                    lstream >> token;
                    c->d.x = std::stof(token);
                    lstream >> token;
                    c->d.y = std::stof(token);
                    lstream >> token;
                    c->d.z = std::stof(token);
                }
            }
            c->a = c->a * transform;
            c->b = c->b * transform;
            c->c = c->c * transform;
            c->a = c->d * transform;
            lstream >> token;
            if (token == w_open) {
            } else {
                throw std::runtime_error("Didn't find open brace");
            }
        } else if (token == w_close && c != NULL) {
            appendToContainer(&unoptimizedObj, c);
            allocCounter++;
            c = NULL;
        } else if (token == w_sphere || token == w_triangle) {
            if (token == w_sphere) {
                Shape *sphere = decodeSphere(line, &tex);
                sphere->s->center = sphere->s->center * transform;
                if (c == NULL) {
                    appendToContainer(&unoptimizedObj, sphere);
                } else {
                    appendToContainer(c, sphere);
                }
                mapStats.spheres++;
                allocCounter += 2; // One for sphere, one for shape container
            } else if (token == w_triangle) {
                Shape *triangle = decodeTriangle(line);
                // std::printf("before %f %f %f\n", triangle->t->a.x, triangle->t->a.y, triangle->t->a.z);
                triangle->t->a = triangle->t->a * transform;
                // std::printf("after %f %f %f\n", triangle->t->a.x, triangle->t->a.y, triangle->t->a.z);
                triangle->t->b = triangle->t->b * transform;
                triangle->t->c = triangle->t->c * transform;
                if (c == NULL) {
                    appendToContainer(&unoptimizedObj, triangle);
                } else {
                    appendToContainer(c, triangle);
                }
                mapStats.tris++;
                allocCounter += 2; // One for triangle, one for shape container
            }
        } else if (token == w_pointLight) {
            PointLight pl = decodePointLight(line);
            pl.center = pl.center * transform;
            pointLights.emplace_back(pl);
            mapStats.lights++;
        } else if (token == w_campreset) {
            CamPreset cp = decodeCamPreset(line);
            camPresets.emplace_back(cp);
        } else if (token == w_comment) {
            continue;
        } else if (token == w_include) {
            lstream >> token;
            std::filesystem::path base = std::filesystem::path(path).parent_path();
           
            std::filesystem::path inc(token);
            std::filesystem::path eval = base / inc;

            Mat4 trans = transform;
            for (int j = 0; j < 3; j++) {
                lstream >> token;
                if (token == w_translate) {
                    Vec3 transl = {0.f, 0.f, 0.f};
                    lstream >> token;
                    transl.x = std::stof(token);
                    lstream >> token;
                    transl.y = std::stof(token);
                    lstream >> token;
                    transl.z = std::stof(token);
                    trans = trans * translateMat(transl);
                } else if (token == w_rotate) {
                    lstream >> token;
                    trans = trans * rotateX(std::stof(token));
                    lstream >> token;
                    trans = trans * rotateY(std::stof(token));
                    lstream >> token;
                    trans = trans * rotateZ(std::stof(token));
                } else if (token == w_scale) {
                    lstream >> token;
                    trans = trans * scale(std::stof(token));
                } else {
                    lstream << " " << token;
                    break;
                }
            }
            loadObjFile(eval.c_str(), trans);
        }
    }
    return allocCounter;
}

void flattenRootContainer(Container *dst, Container *src, bool root) {
    if (src == NULL || src->size == 0) return;
    if (root) {
        dst->a = {1e10, 1e10, 1e30};
        dst->b = {-1e10, -1e10, -1e30};
    }
    Bound *bo = src->start;
    while (bo != src->end->next) {
        Shape *current = bo->s;
        if (current->c != NULL) {
            flattenRootContainer(dst, current->c, false);
        } else {
            Bound *b = emptyBound();
            b->s = emptyShape();
            std::memcpy(b->s, current, sizeof(Shape));
            if (current->s != NULL) {
                b->min = current->s->center - current->s->radius;
                b->max = current->s->center + current->s->radius;
                b->centroid = current->s->center;
                Sphere *s = (Sphere*)alloc(sizeof(Sphere));
                std::memcpy(s, current->s, sizeof(Sphere));
                b->s->s = s;
            }
            if (current->t != NULL) {
                b->min = {9999, 9999, 9999};
                b->max = {-9999, -9999, -9999};
                for (int i = 0; i < 3; i++) {
                    b->min(i) = std::min(b->min(i), current->t->a(i));
                    b->min(i) = std::min(b->min(i), current->t->b(i));
                    b->min(i) = std::min(b->min(i), current->t->c(i));
                    b->max(i) = std::max(b->max(i), current->t->a(i));
                    b->max(i) = std::max(b->max(i), current->t->b(i));
                    b->max(i) = std::max(b->max(i), current->t->c(i));
                }
                b->centroid = 0.333f * (current->t->a + current->t->b + current->t->c);
                Triangle *t = (Triangle*)alloc(sizeof(Triangle));
                std::memcpy(t, current->t, sizeof(Triangle));
                b->s->t = t;
            }
            for (int i = 0; i < 3; i++) {
                dst->a(i) = std::min(dst->a(i), b->min(i));
                dst->b(i) = std::max(dst->b(i), b->max(i));
            }
            
            appendToContainer(dst, b);
        }
        bo = bo->next;
    }
}

WorldMap::~WorldMap() {
    delete cam;
    clearContainer(&unoptimizedObj, true);
    // clearContainer(optimizedObj);
    free(optimizedObj);
    obj = NULL;
    optimizedObj = NULL;
}
