#include "map.hpp"

#include "aa.hpp"
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
    // Generate AA offset array
    int nOffsets = rc->samplesPerPx * rc->samplesPerPx;
    Vec2 *offsets = (Vec2*)alloc(sizeof(Vec2)*nOffsets);
    generateOffsets(offsets, rc->samplesPerPx, rc->sampleMode);
    if (aaOffsetImage != NULL) {
        visualizeOffsets(offsets, rc->samplesPerPx, aaOffsetImage);
        aaOffsetImageDirty = true;
    }
    // TGA::write(aaOffsetImage, "/tmp/test.tga", "test");


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
        threadPtrs[i] = std::thread(&WorldMap::castSubRays, this, img, rc, 0, cam->w, h0, h1, &(rc->threadStates[i]), offsets, nOffsets);
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

// FIXME: Outdated!
/* void WorldMap::createSphere(Vec3 center, float radius, Vec3 color, float opacity, float reflectiveness, float specular, float shininess, float thickness) {
    Shape *sh = emptyShape();
    sh->s = (Sphere*)alloc(sizeof(Sphere));
    sh->s->center = center;
    sh->s->radius = radius;
    sh->s->thickness = thickness;
    sh->m->color = color;
    sh->m->opacity = opacity;
    sh->m->reflectiveness = reflectiveness;
    sh->m->specular = specular;
    sh->m->shininess = shininess;
    appendToContainer(&unoptimizedObj, sh);
} */

// FIXME: Outdated!
/* void WorldMap::createTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float opacity, float reflectiveness, float specular, float shininess) {
    Shape *sh = emptyShape();
    sh->t = (Triangle*)alloc(sizeof(Triangle));
    sh->t->a = a;
    sh->t->b = b;
    sh->t->c = c;
    sh->m->color = color;
    sh->m->opacity = opacity;
    sh->m->reflectiveness = reflectiveness;
    sh->m->specular = specular;
    sh->m->shininess = shininess;
    appendToContainer(&unoptimizedObj, sh);
} */

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
    simpleConfig.normalMapping = rc->normalMapping;
    simpleConfig.reflectanceMapping = rc->reflectanceMapping;
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
        bool hit = r.hit() && !(r.obj->debug) && r.t <= tLight;
        if (hit) {
            continue;
        }
        float scaledDistance = tLight / rc->distanceDivisor;
        float lightBrightness = light.brightness / (scaledDistance*scaledDistance);
        lightColor = lightColor + (light.color * lightBrightness);

        // if (!rc->specular) continue;
        // PHONG : CGPaP in C p.730 Sect. 16.1.4
        // \hat{L} = normDistance,
        // \hat{N} = norm(res->normal),
        // \hat{V} = norm(viewDelta)
        // \hat{R} = rNorm;
        Vec3 rNorm = norm((2.f * res->norm * dot(res->norm, normDistance)) - normDistance);
        // k_s = res->specular,
        // O_{s\varlambda} (Specular Colour) = light.specular * light.specularColor,
        // n (shininess):
        float n = (res->obj->mat()->shininess == -1.f) ? globalShininess : res->obj->mat()->shininess;
        float rDotV = std::fmax(std::pow(dot(rNorm, norm(viewDelta)), n), 0);
        // std::printf("got rDotV %f\n", rDotV);
        // std::printf("specular color %f %f %f\n", light.specularColor.x, light.specularColor.y, light.specularColor.z);
        specularColor = specularColor + (res->obj->mat()->specular * (light.specular * light.specularColor) * rDotV);
    }
    res->specularColor = specularColor;
    // std::printf("%f,%f,%f * %f,%f,%f\n", res->color.x, res->color.y, res->color.z, lightColor.x, lightColor.y, lightColor.z);
    res->lightColor = lightColor;
}

/* void WorldMap::castThroughSphere(Vec3 delta, RenderConfig *rc, RayResult *res, int callCount) {
    float ri = rc->refractiveIndex;
    float thickness = res->sphere.thickness;
    Vec3 center = res->sphere.center;;
    if (thickness == 0.f) {
        // Behave like a filled sphere of air.
        ri = RI_AIR;
        thickness = 1.f;
    }

    Vec3 pb = res->p0;
    Vec3 refract;
    if (thickness == 1.f) {
        castThroughSolidSphere(RI_AIR, ri, res->p0, delta, &(res->sphere), &pb, &refract);
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
            Sphere innerSphere = {center, res->sphere.radius*(1.f - res->sphere.thickness), 1.f};
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
                    t = meetsSphere(pb, refract, &(res->sphere), NULL);
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
                    castThroughSolidSphere(RI_AIR, ri, res->p0, delta, &(res->sphere), &pb, &refract);
                    escaped = true;
                }
            }
        }
    } 
   
    // Finally, cast a ray out from the sphere.
    pb = pb + (EPSILON * refract);
    RayResult r = RayResult();
    castRay(&r, obj, pb, refract, rc, callCount+1);
    res->refractColor = r.color;
} */

// FIXME?:
// An object can span multiple voxels, so to avoid intersecting twice,
// tag an object with the ID of the ray on intersection.
// Then before intersecting an object, check if the object has been intersected by this rayID.
// HOWEVER, unless we used a map per-ray to store intersections or something, this wouldn't work with multiple threads.
// FIXME: Support transforms
void WorldMap::voxelRay(RayResult *res, Container *c, Vec3 p0, Vec3 delta, RenderConfig *rc) {
    delta = norm(delta);
    int x = 0, y = 0, z = 0;
    int subdivision = optimizeLevel;
    Vec3 dims = c->max - c->min;
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
    Vec3 cVoxMin = c->min + Vec3{float(x)*vox.x, float(y)*vox.y, float(z)*vox.z};
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
        Container *c = dynamic_cast<Container*>(o->s);
        bool empty = !(c != nullptr && c->size > 0);
        if (!empty) {
            traversalRay(res, c, p0, delta, rc);
            // the caller, castRay, only casts additional transparency rays if we hit anything behind (i.e. res->collisions > 1), therefore we can only quit if we've hit something solid, or more than 1 object.
            if (res->hit() && (res->obj->mat()->opacity == 1.f || res->collisions > 1)) {
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
    if (c->size == 0) return;
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
        Container *c = dynamic_cast<Container*>(current);
        if (c != nullptr) {
            bool collision = false;
            if (!siblingContainerCollision && accelIndex == Accel::BiTree) {
                // Bi-tree, see comment at top of this method
                collision = true;
            /* } else if (current->c->plane) {
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
            */
            } else {
                // FIXME: Maybe change this in the future?
                // We don't need to apply a transform, as this is a container.
                collision = c->intersects(p0, delta);
                if (!collision) {
                    siblingContainerCollision = false;
                }
                // collision = true;
            }
            // If collision, cast ray to objects within the container
            if (collision) {
                ray(res, c, p0, delta, rc);
            }
        } else {
            // FIXME: Somehow store the if the transformation has been done already,
            // so we don't repeat per ray?
            current->applyTransform();
            Vec3 normal = {0, 0, 0};
            Vec2 uv;
            Vec2 *uvPtr = (current->mat() != NULL && current->mat()->hasTexture()) ? &uv : NULL;
            float t = current->intersect(p0, delta, &normal, uvPtr);
            if (t >= 0) {
                res->collisions++;
                // res->potentialCollisions++;
                if (t < res->t) {
                    res->obj = current;
                    res->t = t;
                    res->p0 = p0 + (res->t * delta);
                    res->normal = normal;
                    res->norm = norm(normal);
                    if (uvPtr != NULL) res->uv = *uvPtr;
                }
            }
        }
        bo = bo->next;
    }
    if (c != &unoptimizable) traversalRay(res, &unoptimizable, p0, delta, rc);
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
    
    if (!res->hit()) return;

    // Shading
    res->color = res->obj->mat()->color;
    
    // Uncomment to visualize normals
    // res->color = (res->norm + 1) / 2.f;

    // std::printf("looped over %d objects\n", size);
    if (rc->collisionsOnly) return;

    if (res->obj->mat()->texId != -1) {
        Texture *tx = tex.at(res->obj->mat()->texId);
        if (tx != NULL) res->color = tx->at(res->uv.x, res->uv.y);
    }
    if (rc->normalMapping && res->obj->mat()->normId != -1) {
        Texture *nm = norms.at(res->obj->mat()->normId);
        if (nm != NULL) {
            Vec3 ts = nm->at(res->uv.x, res->uv.y); // (0,0,1) is pointing towards the normal
            // Change from 0-1 to -1-1
            ts = (ts * 2) - 1;

            // Test
            // ts = {0,0,1};

            // normalmaps are defined in tangent-space, they are relative to themselves. 
            // Find tangent to the sphere (i.e. vector orthogonal to normal)
            
            Vec3 n = res->norm;

            Vec3 a = {0.f, 1.f, 0.f};
            Vec3 anC = cross(a, n);
            // If the normal is vertical, cross(a, n) will be zeros.
            // We just need a vector perp. to a and n, so we just use {1,0,0} instead.
            if (n.x == 0.f && n.z == 0.f && n.y != 0.f) {
                anC = {1.f, 0.f, 0.f};
            }
            anC = norm(anC);
            Vec3 tan = -1.f * anC;
            Vec3 bitan = norm(cross(n, tan));

            // Create TBN matrix
            Mat3 tbn = {
                tan.x, bitan.x, n.x,
                tan.y, bitan.y, n.y,
                tan.z, bitan.z, n.z
            };

            Vec3 sampledNorm = norm(tbn * ts);
            /*if (std::isnan(sampledNorm.y)) {
                std::printf("got nan for (%f %f %f)\n", res->norm.x, res->norm.y, res->norm.z);
            } */
            // std::printf("for normal (%f, %f, %f), got (%f, %f, %f) => (%f, %f, %f)\n", res->norm.x, res->norm.y, res->norm.z, ts.x, ts.y, ts.z, sampledNorm.x, sampledNorm.y, sampledNorm.z);
            res->normal = sampledNorm;
            res->norm = sampledNorm;
        }
    }
    
    /* if (res->normal.y != 0.f && res->normal.x == 0.f && res->normal.z == 0.f) {
        res->color = {1.f, 0.f, 0.f};
    } */
   
    // Epsilon (small number) bias so we don't accidentally hit ourselves due to f.p. 
    Vec3 p0PlusABit = res->p0 + (EPSILON * res->norm);
    float reflectiveness = res->obj->mat()->reflectiveness;
    if (rc->reflections && (reflectiveness != 0 || (rc->reflectanceMapping && res->obj->mat()->refId != -1))) {
        if (rc->reflectanceMapping && res->obj->mat()->refId != -1) {
            Texture *rf = refs.at(res->obj->mat()->refId);
            if (rf != NULL) reflectiveness = rf->at(res->uv.x, res->uv.y)(0); // B&W map, so all channels are identical
        }
        // Angle of reflection: \vec{d} - 2(\vec{d} \cdot \vec{n})\vec{n}
        // FIXME: Put this reflection in a shared function!
        Vec3 reflection = delta - 2.f*dot(delta, res->norm) * res->norm;
        castReflectionRay(p0PlusABit, reflection, rc, res, callCount+1);
    }

    if (rc->lighting && !(res->obj->mat()->noLighting)) {//  && res->obj->reflectiveness != 0) {
        castShadowRays(-1.f*delta, p0PlusABit, rc, res);
    } else {
        res->lightColor = {1.f, 1.f, 1.f};
    }

    // FIXME: AAB opacity and refraction!
    if (res->obj->mat()->opacity != 1.f) {
        // FIXME: Make this more efficient. Maybe don't cast another ray, and just store all collisions data?
        // if (res->collisions >= 1 && res->potentialCollisions > 1) {
        if (res->collisions >= 1) {
            Vec3 p1, delta1;
            res->obj->refract(rc->refractiveIndex, res->p0, delta, &p1, &delta1);
            p1 = p1 + (EPSILON * delta1);
            RayResult r = RayResult();
            castRay(&r, obj, p1, delta1, rc, callCount+1);
            res->refractColor = r.color;
        } else {
            res->refractColor = {0.f, 0.f, 0.f};
        }
    }
 
    
    // Diffuse lighting and specular reflection
    Vec3 out = (res->color * res->lightColor) * (1.f - reflectiveness) + reflectiveness*res->reflectionColor;
    // Interpolated trasnparency (CGPaP 16.25), but not for the specular component
    out = res->obj->mat()->opacity * out + (1.f - res->obj->mat()->opacity) * res->refractColor;
    // Specular highlight
    out = out + res->specularColor;
    
    res->color = out;

    // Clamp
    res->color.x = std::fmin(res->color.x, 1.f);
    res->color.y = std::fmin(res->color.y, 1.f);
    res->color.z = std::fmin(res->color.z, 1.f);
}

void WorldMap::castSubRays(Image *img, RenderConfig *rc, int w0, int w1, int h0, int h1, int *state, Vec2 *offsets, int nOffsets) {
    RayResult res = RayResult();
    Vec3 startCol = w0 * cam->viewportCol;
    Vec3 startRow = h0 * cam->viewportRow;
    Vec3 baseRowVec = cam->viewportCorner + startRow + startCol;

    *state = 1;
    Vec3 accumulatedColor = {0,0,0};
    for (int y = h0; y < h1; y++) {
        // cam->viewportCorner is a vector from the origin, therefore all calculated pixel positions are.
        Vec3 delta = baseRowVec;
        for (int x = w0; x < w1; x++) {
            bool collision = false;
            for (int i = 0; i < nOffsets; i++) {
                Vec3 offsetDelta = delta + (offsets[i].x*cam->viewportCol) + (offsets[i].y*cam->viewportRow);
                res.resetObj();
                castRay(&res, obj, cam->position, offsetDelta, rc);
                if (res.collisions > 0) collision = true;
                accumulatedColor = accumulatedColor + res.color;
            }

            if (collision) {
                img->write(x, y, accumulatedColor/nOffsets);
            }
            accumulatedColor = {0,0,0};
            
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
    const char* w_aab = "box";
    const char* w_pointLight = "plight";
    const char* w_shininess = "shininess";
    const char* w_include = "include";
    const char* w_translate = "translate";
    const char* w_rotate = "rotate";
    const char* w_scale = "scale";
    const char* w_comment = "//";
    const char* w_container = "container";
    const char* w_material = "material";
    const char* w_object = "object";
    const char* w_usingMaterial = "using";
    const char* w_campreset = "campreset";
    const char* w_a = "a";
    const char* w_b = "b";
    const char* w_c = "c";
    const char* w_d = "d";
    const char* w_open = "{";
    const char* w_close = "}";
}

// FIXME: Reimplement!
void WorldMap::encode(char const* path) {
    std::printf("FIXME: Reimplement!\n");
    /* std::ofstream out(path);
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
        if (current->s != NULL) out << dec.encodeSphere(current);
        else if (current->t != NULL) out << dec.encodeTriangle(current);
        bo = bo->next;
    }

    out.close();
    std::printf("Current map saved to %s\n", path); */
}

WorldMap::WorldMap(char const* path) {
    baseBrightness = 0;
    optimizeLevel = 0;
    bvh = true;
    accelIndex = Accel::None;
    accelParam = -1;
    accelFloatParam = 1.5f;
    currentlyRendering = false;
    currentlyOptimizing = false;
    currentlyLoading = false;
    lastRenderTime = 0.f;
    lastOptimizeTime = 0.f;
    lastLoadTime = 0.f;
    obj = &unoptimizedObj;
    flatObj = NULL;
    optimizedObj = NULL;
    camPresetNames = NULL;
    aaOffsetImage = NULL;
    aaOffsetImageDirty = false;
    cam = NULL;
    objectNames = NULL;
    objectPtrs = NULL;
    objectCount = 0;
    dec.setStores(&tex, &norms, &refs, &materials);
    loadFile(path, NULL);
}

void WorldMap::optimizeMap(double (*getTime)(void), int level, int accelIdx) {
    currentlyOptimizing = true;
    optimizeLevel = level;
    accelIndex = accelIdx;
    std::printf("Optimizing map with accelerator %d, depth %d, extra params %d, %f\n", accelIndex, level, accelParam, accelFloatParam);
    if (optimizedObj != NULL) {
        optimizedObj->clear();
        delete optimizedObj;
        optimizedObj = NULL;
    }
    if (flatObj == NULL) {
        flatObj = new Container();
        flattenRootContainer(flatObj, &unoptimizedObj);
        genObjectList(flatObj);
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

void WorldMap::loadFile(char const* path, double (*getTime)(void)) {
    currentlyLoading = true;
    std::printf("Frees: %d\n", unoptimizedObj.clear());
    unoptimizable.clear();
    pointLights.clear();
    camPresets.clear();
    mapStats.clear();
    materials.clear();
    mapStats.name = std::string(path);

    free(camPresetNames);
    if (optimizedObj != NULL) {
        optimizedObj->clear();
        delete optimizedObj;
        optimizedObj = NULL;
    }
    if (flatObj != NULL) {
        flatObj->clear();
        delete flatObj;
        flatObj = NULL;
    }
    obj = &unoptimizedObj;

    tex.clear();
    norms.clear();
    refs.clear();

    if (getTime != NULL) lastLoadTime = getTime();
    loadObjFile(path);
    std::printf("Allocations: %d\n", mapStats.allocs);
    camPresetNames = (const char**)malloc(sizeof(char*)*camPresets.size());
    for (size_t i = 0; i < camPresets.size(); i++) {
        camPresetNames[i] = camPresets.at(i).name.c_str();
    }
    if (getTime != NULL) lastLoadTime = getTime() - lastLoadTime;

    genObjectList(obj);
    materials.genLists();
    tex.genList();
    norms.genList();
    refs.genList();
    
    currentlyLoading = false;
}

void WorldMap::genObjectList(Container *c) {
    free(objectPtrs);
    if (objectNames != NULL) {
        for (int i = 0; i < objectCount; i++) {
            free(objectNames[i]);
        }
        free(objectNames);
    }

    objectCount = mapStats.size();
    objectNames = (char**)malloc(objectCount * sizeof(char*));
    objectPtrs = (Shape **)malloc(objectCount * sizeof(Shape*));
    
    int i = 0;
    for (auto _: pointLights) {
        // "#<num>: Light"
        std::string name = "#" + std::to_string(i) + ": Light";
        // FIXME: This needs to be frreeeeed!
        objectNames[i] = (char*)malloc(sizeof(char)*(name.size()+1));
        strncpy(objectNames[i], name.c_str(), name.size()+1);
        // Lie about what the pointer is
        objectPtrs[i] = (Shape *)&(pointLights.at(i));
        i++;
    }
    // FIXME: This only works on the unoptimized object!
    Bound *bo = c->start;
    while (bo != c->end->next) {
        if (bo->s == NULL) {
            bo = bo->next;
            continue;
        }
        objectPtrs[i] = bo->s;
        std::string name = "#" + std::to_string(i) + ": " + bo->s->name();
        objectNames[i] = (char*)malloc(sizeof(char)*(name.size()+1));
        strncpy(objectNames[i], name.c_str(), name.size()+1);
        bo = bo->next;
        i++;
    }
    if (unoptimizable.size == 0) return;
    bo = unoptimizable.start;
    while (bo != unoptimizable.end->next) {
        if (bo->s == NULL) {
            bo = bo->next;
            continue;
        }
        objectPtrs[i] = bo->s;
        std::string name = "#" + std::to_string(i) + ": " + bo->s->name();
        objectNames[i] = (char*)malloc(sizeof(char)*(name.size()+1));
        strncpy(objectNames[i], name.c_str(), name.size()+1);
        bo = bo->next;
        i++;
    }
}

void WorldMap::loadObjFile(const char* path, Mat4 transform) {
    std::ifstream in(path);
    if (in.fail() || in.bad()) {
        mapStats.missingObj += 1;
        return;
    }
    std::string line;
    Container *c = NULL;
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
        } else if (token == w_material) {
            dec.decodeMaterial(line, true);
            if (tex.lastLoadFail) mapStats.missingTex += 1;
            if (norms.lastLoadFail) mapStats.missingNorm += 1;
            if (refs.lastLoadFail) mapStats.missingRef += 1;
            // mapStats.materials++;
        } else if (token == w_usingMaterial) {
            token = collectWordOrString(lstream);
            dec.usingMaterial(token);
            lstream >> token;
            if (token == w_open) {
            } else {
                throw std::runtime_error("Didn't find open brace");
            }
        /*} else if (token == w_container) {
            c = new Container(true);
            mapStats.allocs++;
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
        */
        } else if (token == w_close) {
            if (c != NULL) {
                mapStats.allocs += unoptimizedObj.append(c);
                c = NULL;
            } else if (dec.isUsingMaterial()) {
                dec.endUsingMaterial();
            }
        } else if (token == w_sphere || token == w_triangle || token == w_aab) {
            if (token == w_sphere) {
                Sphere *sphere = dec.decodeSphere(line);
                if (tex.lastLoadFail) mapStats.missingTex += 1;
                if (norms.lastLoadFail) mapStats.missingNorm += 1;
                if (refs.lastLoadFail) mapStats.missingRef += 1;
                mapStats.allocs += 1; // Sphere
                // FIXME: Scale isn't applied to the radius!
                sphere->oCenter = sphere->oCenter * transform;
                if (c == NULL) {
                    mapStats.allocs += unoptimizedObj.append(sphere);
                } else {
                    mapStats.allocs += c->append(sphere);
                }
                mapStats.spheres++;
            } else if (token == w_triangle) {
                Triangle *triangle = dec.decodeTriangle(line);
                if (tex.lastLoadFail) mapStats.missingTex += 1;
                if (norms.lastLoadFail) mapStats.missingNorm += 1;
                if (refs.lastLoadFail) mapStats.missingRef += 1;
                mapStats.allocs += 1; // Triangle
                triangle->oA = triangle->oA * transform;
                triangle->oB = triangle->oB * transform;
                triangle->oC = triangle->oC * transform;
                if (triangle->plane) {
                    mapStats.allocs += unoptimizable.append(triangle);
                    mapStats.planes++;
                } else {
                    if (c == NULL) {
                        mapStats.allocs += unoptimizedObj.append(triangle);
                    } else {
                        mapStats.allocs += c->append(triangle);
                    }
                    mapStats.tris++;
                }
            } else if (token == w_aab) {
                AAB *aab = dec.decodeAAB(line);
                if (tex.lastLoadFail) mapStats.missingTex += 1;
                if (norms.lastLoadFail) mapStats.missingNorm += 1;
                if (refs.lastLoadFail) mapStats.missingRef += 1;
                mapStats.allocs += 1; // AAB
                aab->oMin = aab->oMin * transform;
                aab->oMax = aab->oMax * transform;
                if (c == NULL) {
                    mapStats.allocs += unoptimizedObj.append(aab);
                } else {
                    mapStats.allocs += c->append(aab);
                }
                mapStats.aabs++;
            }
        } else if (token == w_pointLight) {
            PointLight pl = dec.decodePointLight(line);
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
                    trans = trans * transformScale(std::stof(token));
                } else {
                    lstream << " " << token;
                    break;
                }
            }
            loadObjFile(eval.c_str(), trans);
        }
    }
}

void flattenRootContainer(Container *dst, Container *src, bool root) {
    if (src == NULL || src->size == 0) return;
    if (root) {
        dst->min = {1e10, 1e10, 1e30};
        dst->max = {-1e10, -1e10, -1e30};
    }
    Bound *bo = src->start;
    while (bo != src->end->next) {
        Shape *current = bo->s;
        Container *c = dynamic_cast<Container*>(current);
        if (c != nullptr) {
            flattenRootContainer(dst, c, false);
        } else {
            Bound *b = emptyBound();
            // FIXME: Make sure this works!
            b->s = current->clone();
            current->bounds(b);
            for (int i = 0; i < 3; i++) {
                dst->min(i) = std::min(dst->min(i), b->min(i));
                dst->max(i) = std::max(dst->max(i), b->max(i));
            }
            dst->append(b);
        }
        bo = bo->next;
    }
}

WorldMap::~WorldMap() {
    delete cam;
    unoptimizedObj.clear();
    unoptimizable.clear();
    // FIXME: These might be issues?
    flatObj->clear();
    delete flatObj;
    optimizedObj->clear();
    delete optimizedObj;
}
