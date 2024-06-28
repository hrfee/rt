#include "test.hpp"
#include "util.hpp"
#include "ray.hpp"
#include "vec.hpp"
#include <chrono>
#include <random>
#include <vector>

Triangle *triList(int n, int *seed) {
    int genSeed = 0;
    if (seed == NULL) {
        seed = &genSeed;
    }
    if (*seed == -9998) {
        std::random_device rd;
        *seed = rd();
    }
    std::mt19937 gen(*seed);
    std::uniform_int_distribution<> distrib(-9998, 9999);

    Triangle *t = (Triangle*)alloc(sizeof(Triangle)*n);
    if (t == NULL) return t;
    for (int i = 0; i < n; i++) {
        // Triangle is just Vec3 a, b, c, so just treat it as a 9 float array.
        for (int j = 0; j < 9; j++) {
            float *v = ((float*)(t+i))+j;
            *v = float(distrib(gen)) / 1000.f;
        }
    }
    return t;
}

Sphere *sphereList(int n, int *seed) {
    int genSeed = 0;
    if (seed == NULL) {
        seed = &genSeed;
    }
    if (*seed == -9998) {
        std::random_device rd;
        *seed = rd();
    }
    std::mt19937 gen(*seed);
    std::uniform_int_distribution<> center(-9998, 9999);
    std::uniform_int_distribution<> radius(1, 9999);

    Sphere *t = (Sphere*)alloc(sizeof(Sphere)*n);
    if (t == NULL) return t;
    for (int i = 0; i < n; i++) {
        // Similar thing as triList, but just for the center Vec3.
        for (int j = 0; j < 3; j++) {
            float *v = ((float*)(t+i))+j;
            *v = float(center(gen)) / 1000.f;
        }
        (t+i)->radius = float(radius(gen)) / 1000.f;
        (t+i)->thickness = 1.f;
    }
    return t;
}

struct SimpleBound {
    Vec3 min, max;
};

double traverseAll(Triangle *t, int n) {
    std::vector<Vec3> rays;
   
    // Generate random points in the bounding-boxes of each triangle, which we'll use as our ray.
    std::random_device rd;
    auto seed = rd();
    std::mt19937 gen(seed);

    for (int i = 0; i < n; i++) {
        Triangle *tri = t+i;
        SimpleBound b = {{9999, 9999, 9999}, {-9999, -9999, -9999}};
        Vec3 ray = {0,0,0};
        for (int j = 0; j < 3; j++) {
            b.min(j) = std::min(b.min(j), tri->a(j));
            b.min(j) = std::min(b.min(j), tri->b(j));
            b.min(j) = std::min(b.min(j), tri->c(j));
            b.max(j) = std::max(b.max(j), tri->a(j));
            b.max(j) = std::max(b.max(j), tri->b(j));
            b.max(j) = std::max(b.max(j), tri->c(j));
            std::uniform_real_distribution<> distrib(b.min(j), b.max(j));
            ray(j) = distrib(gen);
        }
        rays.push_back(ray);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        Triangle *tri  = t+i;
        Vec3 ray = rays.at(i);
        Vec3 normal;
        /*float t = */tri->intersect({0,0,0}, ray, &normal);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = end - start;
    return ms.count();
}

double traverseAll(Sphere *s, int n) {
    std::vector<Vec3> rays;
   
    // Generate random points in the bounding-boxes of each sphere, which we'll use as our ray.
    std::random_device rd;
    auto seed = rd();
    std::mt19937 gen(seed);

    for (int i = 0; i < n; i++) {
        Sphere *sphere = s+i;
        SimpleBound b = {
            sphere->center - sphere->radius,
            sphere->center + sphere->radius
        };
        Vec3 ray = {0,0,0};
        for (int j = 0; j < 3; j++) {
            std::uniform_real_distribution<> distrib(b.min(j), b.max(j));
            ray(j) = distrib(gen);
        }
        rays.push_back(ray);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        Sphere *sphere = s+i;
        Vec3 ray = rays.at(i);
        Vec3 normal;
        /*float t = */sphere->intersect({0,0,0}, ray, &normal);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = end - start;
    return ms.count();
}
