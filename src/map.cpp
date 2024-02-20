#include "map.hpp"

#include "util.hpp"
#include "ray.hpp"
#include <cmath>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>

#define RI_AIR 1.f
#define RI_GLASS 1.52f

ContainerQuad *emptyContainerQuad() {
    ContainerQuad *c = (ContainerQuad*)alloc(sizeof(ContainerQuad));
    c->a = {0, 0 ,0};
    c->b = {0, 0, 0};
    c->c = {0, 0, 0};
    c->d = {0, 0, 0};
    c->start = NULL;
    c->end = NULL;
    return c;
}

void appendShape(ContainerQuad *c, Shape *sh) {
    if (c->start == NULL) {
        c->start = sh;
    }
    if (c->end != NULL) c->end->next = sh;
    c->end = sh;
}

void appendContainerQuad(ContainerQuad *cParent, ContainerQuad *c) {
    Shape *sh = emptyShape();
    sh->c = c;
    appendShape(cParent, sh);
}

void WorldMap::createSphere(Vec3 center, float radius, Vec3 color, float opacity, float reflectiveness, float specular, float shininess) {
    Shape *sh = emptyShape();
    sh->s = (Sphere*)alloc(sizeof(Sphere));
    sh->s->center = center;
    sh->s->radius = radius;
    sh->color = color;
    sh->opacity = opacity;
    sh->reflectiveness = reflectiveness;
    sh->specular = specular;
    sh->shininess = shininess;
    appendShape(&o, sh);
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
    appendShape(&o, sh);
}

void WorldMap::createDebugVector(Vec3 p0, Vec3 delta, Vec3 color) {
    Vec3 p1 = p0 + delta;
    Shape *sh = emptyShape();
    sh->t = (Triangle*)alloc(sizeof(Triangle));
    sh->t->a = p0;
    sh->t->a.y -= 0.5f;
    sh->t->b = p0;
    sh->t->b.y += 0.5f;
    sh->t->c = p1;
    sh->color = color;
    sh->opacity = 1.f;
    sh->reflectiveness = 0.f;
    sh->specular = 0.f;
    sh->shininess = 5.f;
    appendShape(&debug, sh);
}

void WorldMap::castRays(Image *img, RenderConfig *rc) {
    Vec3 baseRowVec = cam->viewportCorner;
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
    for (int y = 0; y < cam->h; y++) {
        // cam->viewportCorner is a vector from the origin, therefore all calculated pixel positions are.
        Vec3 delta = baseRowVec;
        for (int x = 0; x < cam->w; x++) {
            RayResult res = castRay(&o, cam->position, delta, rc);
            if (res.collisions > 0) {
                writePixel(img, x, y, res.color);
            }
            res = castRay(&debug, cam->position, delta, rc);
            if (res.collisions > 0) {
                writePixel(img, x, y, res.color);
            }
            
            delta = delta + cam->viewportCol; 
        }
        baseRowVec = baseRowVec + cam->viewportRow;
    }
    clearContainer(&debug);
}

void WorldMap::castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount) {
    RayResult bounce = castRay(&o, p0, delta, rc, callCount);
    Vec3 color = {0.f, 0.f, 0.f};
    if (bounce.collisions > 0) {
        color = bounce.color;
    }
    res->reflectionColor = color;
}

void WorldMap::castShadowRays(Vec3 viewDelta, Vec3 p0, RenderConfig *rc, RayResult *res) {
    // FIXME: Consider transparent obstacles when those are added.
    Vec3 lightColor = Vec3{1.f, 1.f, 1.f} * rc->baseBrightness;
    Vec3 specularColor = Vec3{0.f, 0.f, 0.f};
    RenderConfig simpleConfig;
    simpleConfig.lighting = false;
    simpleConfig.reflections = false;
    simpleConfig.triangles = rc->triangles;
    simpleConfig.spheres = rc->spheres;
    for (PointLight light: pointLights) {
        Vec3 distance = light.center - p0;
        float tLight = mag(distance);
        Vec3 normDistance = distance / tLight;
        RayResult r = castRay(&o, p0, normDistance, &simpleConfig, 0);
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
        float n = (res->shininess == -1.f) ? globalShininess : res->shininess;
        float rDotV = std::fmax(std::pow(dot(rNorm, norm(viewDelta)), n), 0);
        // std::printf("got rDotV %f\n", rDotV);
        // std::printf("specular color %f %f %f\n", light.specularColor.x, light.specularColor.y, light.specularColor.z);
        specularColor = specularColor + (res->specular * (light.specular * light.specularColor) * rDotV);
    }
    res->specularColor = specularColor;
    res->color = (res->color * lightColor);
}

RayResult WorldMap::castRay(ContainerQuad *c, Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount) {
    RayResult res = {0, 0, 9999.f, 9999.f};
    if (callCount > rc->maxBounce) return res;
    Shape *current = c->start;
    while (current != NULL) {
        if (current->s != NULL && rc->spheres) {
            Sphere *sphere = current->s;
            float t1 = -1.f;
            float t = meetsSphere(p0, delta, sphere, &t1);
            if (t >= 0) {
                res.collisions += 1;
                res.potentialCollisions += 1;
            }
            if (t >= 0 && t < res.t) {
                res.obj = current;
                res.t = t;
                res.t1 = t1;
                res.color = current->color;
                res.opacity = current->opacity;
                res.reflectiveness = current->reflectiveness;
                res.shininess = current->shininess;
                Vec3 collisionPoint = p0 + (res.t * delta);
                res.normal = collisionPoint-sphere->center;
                res.norm = norm(res.normal);
                res.p0 = collisionPoint;
                res.specular = current->specular;
            }
        } else if (current->t != NULL && rc->triangles) {
            // FIXME: Triangles with identical X coords do not render!
            Triangle *tri = current->t;
            Vec3 normal = getVisibleTriNormal(delta, tri->a, tri->b, tri->c);
            Vec3 normnorm = norm(normal);
            float t = meetsTrianglePlane(p0, delta, normnorm, tri);
            if (t > 0) {
                res.potentialCollisions += 1;
                if (t < res.t) {
                    Vec3 collisionPoint = p0 + (t * delta);
                    if (meetsTriangle(normnorm, collisionPoint, tri)) {
                        res.collisions += 1;
                        res.obj = current;
                        res.t = t;
                        res.color = current->color;
                        res.opacity = current->opacity;
                        res.reflectiveness = current->reflectiveness;
                        res.shininess = current->shininess;
                        res.normal = normal;
                        res.norm = normnorm;
                        res.p0 = collisionPoint;
                        res.specular = current->specular;
                    } else {
                        res.potentialCollisions -= 1;
                    }
                }
            }
        }
        if (current->c != NULL) {
            // Test container collision
            Triangle tris[2] = {
                {current->c->a, current->c->b, current->c->c},
                {current->c->c, current->c->d, current->c->a},
            };
            bool collision = !(rc->planeOptimisation);
            if (!collision) {
                for (int i = 0; i < 2; i++) {
                    Vec3 normal = norm(getVisibleTriNormal(delta, tris[i].a, tris[i].b, tris[i].c));
                    float t = meetsTrianglePlane(p0, delta, normal, &(tris[i]));
                    if (t < 0 || t >= res.t) continue;
                    Vec3 collisionPoint = p0 + (t * delta);
                    if (meetsTriangle(normal, collisionPoint, &(tris[i]))) {
                        collision = true;
                        break;
                    }
                }
            }
            // If collision, cast rays to all objects in the container
            if (collision) {
                RenderConfig simpleConfig;
                simpleConfig.lighting = false;
                simpleConfig.reflections = false;
                simpleConfig.triangles = rc->triangles;
                simpleConfig.spheres = rc->spheres;
                RayResult r = castRay(current->c, p0, delta, &simpleConfig, 0);
                res.collisions += r.collisions;
                res.potentialCollisions += r.potentialCollisions;
                if (r.t > 0 && r.t < res.t) {
                    res.obj = r.obj;
                    res.t = r.t;
                    res.color = r.color;
                    res.opacity = current->opacity;
                    res.reflectiveness = r.reflectiveness;
                    res.shininess = r.shininess;
                    res.normal = r.normal;
                    res.norm = r.norm;
                    res.p0 = r.p0;
                    res.specular = r.specular;
                }
            }
        }
        current = current->next;
    }
    // FIXME: This shouldn't be necessary, but without it, bouncing rays collide with the sphere they bounce off, causing a weird moiré pattern. To avoid, this moves the origin just further than the edge of the sphere.
    Vec3 p0PlusABit = res.p0 + (0.001f * res.normal);
    if (rc->reflections && res.reflectiveness != 0) {
        // Angle of reflection: \vec{d} - 2(\vec{d} \cdot \vec{n})\vec{n}
        Vec3 reflection = delta - 2.f*dot(delta, res.norm) * res.norm;
        castReflectionRay(p0PlusABit, reflection, rc, &res, callCount+1);
        res.color = ((1.f - res.reflectiveness)*res.color);
    }

    if (rc->lighting && res.reflectiveness != 0) {
        castShadowRays(-1.f*delta, p0PlusABit, rc, &res);
    }

    if (res.opacity != 1.f) {
        // FIXME: Make this more efficient. Maybe don't cast another ray, and just store all collisions data?
        if (res.collisions >= 1 && res.potentialCollisions > 1) {
            Vec3 p0Transparent = res.p0 - (0.001f * res.normal);
            if (res.obj->t != NULL) {
                RayResult r = castRay(&o, p0Transparent, delta, rc, callCount+1);
                res.refractColor = r.color;
            } else if (res.obj->s != NULL) {
                // 1. Bounce into the sphere.
                bool tir = false;
                Vec3 refract = Refract(RI_AIR, rc->refractiveIndex, delta, res.normal, &tir);

                if (tir) {
                    // Don't do anything, the sphere should be reflective so it'll look fine.
                    // std::printf("TIR!\n");
                    // res.color = {0.f, 1.f, 0.f};
                } else {
                    // 2. Bounce around the glass until we can escape.
                    tir = true;
                    Vec3 pb = res.p0;
                    while (tir) {
                        pb = pb + (0.001f * refract);
                        float t = meetsSphere(pb, refract, res.obj->s, NULL);
                        pb = pb + (t * refract);
                        // Since we're inside the sphere, invert the normal to point inwards
                        Vec3 normal = -1.f*(pb - res.obj->s->center);
                        refract = Refract(rc->refractiveIndex, RI_AIR, refract, normal, &tir);
                    }
                    pb = pb + (0.001f * refract);
                    RayResult r = castRay(&o, pb, refract, rc, callCount+1);
                    res.refractColor = r.color;
                }
            }
        } else {
            res.refractColor = {0.f, 0.f, 0.f};
        }
    }
   
    res.color = (res.opacity * res.color) + ((1.f - res.opacity) * res.refractColor) + (res.reflectiveness * res.reflectionColor) + res.specularColor;

    res.color.x = std::fmin(res.color.x, 1.f);
    res.color.y = std::fmin(res.color.y, 1.f);
    res.color.z = std::fmin(res.color.z, 1.f);
    return res;
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
    const char* w_comment = "//";
    const char* w_container = "container";
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
    Shape *current = o.start;
    while (current != NULL) {
        if (current->s != NULL) out << encodeSphere(current);
        else if (current->t != NULL) out << encodeTriangle(current);
        current = current->next;
    }

    out.close();
    std::printf("Current map saved to %s\n", path);
}

WorldMap::WorldMap(char const* path) {
    o.start = NULL;
    o.end = NULL;
    loadFile(path);
}

void WorldMap::loadFile(char const* path) {
    // FIXME: Reloading the map causes a SIGSEGV, due to accessing a free'd pointer maybe?
    std::printf("Frees: %d\n", clearContainer(&o));
    pointLights.clear();
    std::printf("Allocations: %d\n", loadObjFile(path));
}

int WorldMap::loadObjFile(const char* path) {
    std::ifstream in(path);
    std::string line;
    ContainerQuad *c = NULL;
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
            c = emptyContainerQuad();
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
            lstream >> token;
            if (token == w_open) {
            } else {
                throw std::runtime_error("Didn't find open brace");
            }
        } else if (token == w_close && c != NULL) {
            appendContainerQuad(&o, c);
            allocCounter++;
            c = NULL;
        } else if (token == w_sphere || token == w_triangle) {
            if (token == w_sphere) {
                if (c == NULL) {
                    appendShape(&o, decodeSphere(line));
                } else {
                    appendShape(c, decodeSphere(line));
                }
                allocCounter += 2; // One for sphere, one for shape container
            } else if (token == w_triangle) {
                if (c == NULL) {
                    appendShape(&o, decodeTriangle(line));
                } else {
                    appendShape(c, decodeTriangle(line));
                }
                allocCounter += 2; // One for triangle, one for shape container
            }
        } else if (token == w_pointLight) {
            pointLights.emplace_back(decodePointLight(line));
        } else if (token == w_comment) {
            continue;
        } else if (token == w_include) {
            lstream >> token;
            std::filesystem::path base = std::filesystem::path(path).parent_path();
            
            std::filesystem::path inc(token);
            std::filesystem::path eval = base / inc;
            loadObjFile(eval.c_str());
        }
    }
    return allocCounter;
}

int clearContainer(ContainerQuad *c) {
    Shape *current = c->start;
    c->start = NULL;
    c->end = NULL;
    int freeCounter = 0;
    while (current != NULL) {
        if (current->s != NULL) {
            free(current->s);
            current->s = NULL;
            freeCounter++;
        }
        if (current->t != NULL) {
            free(current->t);
            current->t = NULL;
            freeCounter++;
        }
        if (current->c != NULL) {
            freeCounter += clearContainer(current->c);
            free(current->c);
            current->c = NULL;
            freeCounter++;
        }
        Shape *next = current->next;
        free(current);
        freeCounter++;
        current = next;
    }
    return freeCounter;
}

WorldMap::~WorldMap() {
    delete cam;
    clearContainer(&o);
}
