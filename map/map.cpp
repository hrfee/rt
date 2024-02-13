#include "map.hpp"

#include "../vec/mat.hpp"
#include <cmath>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <filesystem>

void WorldMap::appendShape(Shape *sh) {
    sh->next = NULL;
    if (start == NULL) start = sh;
    if (end != NULL) end->next = sh;
    end = sh;
}

void WorldMap::appendSphere(Sphere *s) {
    Shape *sh = (Shape*)malloc(sizeof(Shape)); // FIXME: Error check!
    sh->s = s;
    sh->t = NULL;
    appendShape(sh);
}

void WorldMap::appendSphere(Vec3 center, float radius, Vec3 color, float reflectiveness, float specular, float shininess) {
    Sphere *s = (Sphere*)malloc(sizeof(Sphere)); // FIXME: Error check!
    s->center = center;
    s->radius = radius;
    s->color = color;
    s->reflectiveness = reflectiveness;
    s->specular = specular;
    s->shininess = shininess;
    Shape *sh = (Shape*)malloc(sizeof(Shape)); // FIXME: Error check!
    sh->s = s;
    sh->t = NULL;
    appendShape(sh);
}

void WorldMap::appendTriangle(Triangle *t) {
    Shape *sh = (Shape*)malloc(sizeof(Shape)); // FIXME: Error check!
    sh->t = t;
    sh->s = NULL;
    appendShape(sh);
}

void WorldMap::appendTriangle(Vec3 a, Vec3 b, Vec3 c, Vec3 color, float reflectiveness, float specular, float shininess) {
    Triangle *t = (Triangle*)malloc(sizeof(Triangle)); // FIXME: Error check!
    t->a = a;
    t->b = b;
    t->c = c;
    t->color = color;
    t->reflectiveness = reflectiveness;
    t->specular = specular;
    t->shininess = shininess;
    Shape *sh = (Shape*)malloc(sizeof(Shape)); // FIXME: Error check!
    sh->t = t;
    sh->s = NULL;
    appendShape(sh);
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
            RayResult res = castRay(cam->position, delta, rc);
            if (res.collisions > 0) {
                writePixel(img, x, y, res.color);
            }
            
            delta = delta + cam->viewportCol; 
        }
        baseRowVec = baseRowVec + cam->viewportRow;
    }
}

// CGPaP in C p.339 Sect. 7.12.2 "PtInPolygon":
// Cast a ray from our point in any direction.
// If the ray hits an edge once, we're inside. If twice, we're outside.
// Uses cramer's rule to find point where ray meets edge.
bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    // std::printf("tri %f %f %f %f %f %f\n", a.x, a.y, b.x, b.y, c.x, c.y);
    // std::printf("point %f %f\n", p.x, p.y);
    int collisions = 0;
    float maxX = std::max(std::max(a.x, b.x), c.x);
    float minX = std::min(std::min(a.x, b.x), c.x);
    float maxY = std::max(std::max(a.y, b.y), c.y);
    float minY = std::min(std::min(a.y, b.y), c.y);
    // Easy case: point is outside triangle's bounding box.
    // These aren't full-fledged polygons so they aren't gonna have
    // a hole in the middle.
    if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY) {
        return false;
    }
    // Cast a ray across the x axis, p + q, where q=(maxX+1, p.y) - p:
    Vec2 q = Vec2{maxX+1.f, p.y} - p;
    Vec2 edges[3][2] = {{a, b}, {b, c}, {c, a}};
    for (int i = 0; i < 3; i++) {
        Vec2 r = edges[i][0];
        Vec2 s = edges[i][1] - r;
        // Find where p + tq = r + us, using cramer's rule:
        Mat2 d = Mat2{
            q.x, -s.x,
            q.y, -s.y
        };
        Mat2 dt = Mat2{
            r.x - p.x, -s.x,
            r.y - p.y, -s.y
        };
        Mat2 du = Mat2{
            q.x, r.x - p.x,
            q.y, r.y - p.y
        };
        float detD = det(d);
        float t = det(dt)/detD;
        float u = det(du)/detD;
        if (detD != 0 && t >= 0.f && u >= 0.f && t <= 1.f && u <= 1.f) {
            // Vec2 g = p + (t * q);
            // Vec2 j = r + (u * s);
            // std::printf("collision on %d ((%f %f) + t(%f %f) = (%f %f) + u(%f %f))! t=%f (%f, %f), u=%f (%f, %f)\n", i, p.x, p.x, q.x, q.y, r.x, r.y, s.x, s.y, t, g.x, g.y, u, j.x, j.y);
            collisions += 1;
        }
        if (collisions == 2) break;
    }
    // if (collisions == 1) {
    //     std::printf("in triangle!\n");
    // }
    return collisions == 1;
}

// Finds the normal of a triangle (abc) that points towards given vector p0
Vec3 getVisibleTriNormal(Vec3 v0, Vec3 a, Vec3 b, Vec3 c) {
    Vec3 norms[2] = {cross(a - c, b - c), cross(c - a, b - a)};
    float components[2] = {0, 0};
    for (int i = 0; i < 2; i++) {
        // Roughly computing component of each normal along vector v0,
        // a.k.a (n_i \dot v0)/|v0|.
        // but since we just want the lowest one (most negative), the denom. can be ignored.
        components[i] = dot(norms[i], v0);
    }
    if (components[0] <= components[1])
        return norms[0];
    return norms[1];
}

void WorldMap::castReflectionRay(Vec3 p0, Vec3 delta, RenderConfig *rc, RayResult *res, int callCount) {
    RayResult bounce = castRay(p0, delta, rc, callCount);
    Vec3 color = {0.f, 0.f, 0.f};
    if (bounce.collisions > 0) {
        color = bounce.color;
    }
    res->color = ((1.f - res->reflectiveness)*res->color + (res->reflectiveness) * color);
    if (!(rc->lighting)) {
        return;
    }
}

float meetsSphere(Vec3 p0, Vec3 delta, Sphere sphere) {
    // CG:PaP 2nd ed. in C, p. 703, eq. 15.17 is an expanded sphere equation with
    // substituted values for the camera position (x/y/z_0),
    // pixel vec from camera (delta x/y/z),
    // and normalized distance along pixel vector (t).
    Vec3 originToSphere = {p0.x - sphere.center.x, p0.y - sphere.center.y, p0.z - sphere.center.z};
    float a = dot(delta, delta);
    float b = 2.f * (delta.x*originToSphere.x + delta.y*originToSphere.y + delta.z*originToSphere.z);
    float c = dot(originToSphere, originToSphere) - sphere.radius*sphere.radius;
    float discrim = b*b - 4.f*a*c;
    if (discrim < 0) return -1;
    float discrimRoot = std::sqrt(discrim);
    float t = (-b - discrimRoot) / (2.f*a);
    if (discrim > 0 && t < 0) {
        float t1 = (-b + discrimRoot) / (2.f*a);
        if (t < 0) t = t1;
    }
    return t;
}

float meetsTrianglePlane(Vec3 p0, Vec3 delta, Vec3 normal, Triangle tri) {
    // dot product of a line on the plane with the normal is zero, hence (p - t.a) \dot norm = 0, where p is a random point (x, y, z)
    // p \dot norm = t.a \dot norm
    // compute the right side, expand the left side and subtract it:
    // (norm.x)x + (norm.y)y + (norm.z)z - (t.a \dot norm) = 0
    // Take values in equation of a plane (CGPaP in C p. 703 eq. 15.18): Ax + By + Cz + D = 0
    // where A = norm.x, B = norm.y, C = norm.z
    float d = -dot(tri.a, normal);
    // CGPaP in C p.703 eq. 15.21, using dot products to make more readable
    float t = -(dot(normal, p0) + d);
    float denom = dot(normal, delta);
    if (denom == 0) { // Plane parallel to ray, ignore
        return -1;
    }
    return t / denom;
}

bool meetsTriangle(Vec3 normal, Vec3 collisionPoint, Triangle tri) {
    // project orthographically as big as possible
    Vec2 ao, bo, co, po;
    if (normal.x >= normal.y && normal.x >= normal.z) {
        // std::printf("projecting onto x\n");
        ao = Vec2{tri.a.y, tri.a.z};
        bo = Vec2{tri.b.y, tri.b.z};
        co = Vec2{tri.c.y, tri.c.z};
        po = Vec2{collisionPoint.y, collisionPoint.z};
    } else if (normal.y >= normal.x && normal.y >= normal.z) {
        // std::printf("projecting onto y\n");
        ao = Vec2{tri.a.x, tri.a.z};
        bo = Vec2{tri.b.x, tri.b.z};
        co = Vec2{tri.c.x, tri.c.z};
        po = Vec2{collisionPoint.x, collisionPoint.z};
    } else { 
        // std::printf("projecting onto z\n");
        ao = Vec2{tri.a.x, tri.a.y};
        bo = Vec2{tri.b.x, tri.b.y};
        co = Vec2{tri.c.x, tri.c.y};
        po = Vec2{collisionPoint.x, collisionPoint.y};
    }

    return pointInTriangle(po, ao, bo, co);
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
        RayResult r = castRay(p0, normDistance, &simpleConfig, 0);
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
    res->color = (res->color * lightColor) + specularColor;
}

RayResult WorldMap::castRay(Vec3 p0, Vec3 delta, RenderConfig *rc, int callCount) {
    RayResult res = {0, 9999.f, 9999.f};
    if (callCount > MAX_BOUNCE) return res;
    Shape *current = start;
    while (current != NULL) {
        if (current->s != NULL && rc->spheres) {
            Sphere *sphere = current->s;
            float t = meetsSphere(p0, delta, *sphere);
            if (t >= 0) res.collisions += 1;
            if (t >= 0 && t < res.t) {
                res.t = t;
                res.color = sphere->color;
                res.reflectiveness = sphere->reflectiveness;
                res.shininess = sphere->shininess;
                Vec3 collisionPoint = p0 + (res.t * delta);
                res.normal = collisionPoint-sphere->center;
                res.p0 = collisionPoint;
                res.specular = sphere->specular;
            }
        } else if (current->t != NULL && rc->triangles) {
            Triangle *tri = current->t;
            Vec3 normal = norm(getVisibleTriNormal(delta, tri->a, tri->b, tri->c));
            float t = meetsTrianglePlane(p0, delta, normal, *tri);
            if (t > 0 && t < res.t) {
                Vec3 collisionPoint = p0 + (t * delta);
                if (meetsTriangle(normal, collisionPoint, *tri)) {
                    res.collisions += 1;
                    res.t = t;
                    res.color = tri->color;
                    res.reflectiveness = tri->reflectiveness;
                    res.shininess = tri->shininess;
                    res.normal = normal;
                    res.p0 = collisionPoint;
                    res.specular = tri->specular;
                }
            }
        }
        current = current->next;
    }
    // FIXME: This shouldn't be necessary, but without it, bouncing rays collide with the sphere they bounce off, causing a weird moiré pattern. To avoid, this moves the origin just further than the edge of the sphere.
    res.p0 = res.p0 + (0.001f * res.normal);
    if (rc->reflections && res.reflectiveness != 0) {
        castReflectionRay(res.p0, res.normal, rc, &res, callCount+1);
    }
    if (rc->lighting && res.reflectiveness != 0) {
        castShadowRays(-1.f*delta, res.p0, rc, &res);
    }

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
    Shape *current = start;
    while (current != NULL) {
        if (current->s != NULL) out << encodeSphere(current->s);
        else if (current->t != NULL) out << encodeTriangle(current->t);
        current = current->next;
    }

    out.close();
    std::printf("Current map saved to %s\n", path);
}

WorldMap::WorldMap(char const* path) {
    start = NULL;
    end = NULL;
    loadFile(path);
}

void WorldMap::loadFile(char const* path) {
    clearShapes();
    pointLights.clear();
    loadObjFile(path);
}

void WorldMap::loadObjFile(const char* path) {
    std::ifstream in(path);
    std::string line;
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
        } else if (token == w_sphere || token == w_triangle) {
            if (token == w_sphere) {
                appendSphere(decodeSphere(line));
            } else if (token == w_triangle) {
                appendTriangle(decodeTriangle(line));
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
}

void WorldMap::clearShapes() {
    if (start == NULL) return;
    Shape *current = start;
    while (current != NULL) {
        if (current->s != NULL) free(current->s);
        if (current->t != NULL) free(current->t);
        Shape *next = current->next;
        free(current);
        current = next;
    }
}

WorldMap::~WorldMap() {
    delete cam;
    clearShapes();
}
