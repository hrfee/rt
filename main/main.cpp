#include "../map/map.hpp"
#include "../img/img.hpp"
#include "../render_bmp/bmp.hpp"
#include "../render_gl/gl.hpp"
#include "../cam/cam.hpp"
#include <math.h>
#include <cstdio>

#define TEST_W 1920 
#define TEST_H 1080
#define TEST_RES_MULTIPLIER .5f
#define TEST_FOV 60

namespace {
    constexpr int initialWidth = int(float(TEST_W)*TEST_RES_MULTIPLIER);
    constexpr int initialHeight = int(float(TEST_H)*TEST_RES_MULTIPLIER);
    constexpr float initialFOV = float(TEST_FOV)*M_PI / 180.f;
    WorldMap *map;
    int renderWidth = initialWidth;
    int renderHeight = initialHeight;
    Image *img;
    GLWindow window(renderWidth, renderHeight);
}

Image *mainLoop() {
    clearImage(img);
    map->castRays(img);
    return img;
}

int main(void) {
    map = new WorldMap(50.f, 50.f, 50.f);

    map->cam = new Camera(renderWidth, renderHeight, initialFOV, {0.f, 0.f, 0.f});
    map->cam->theta = 0.f * M_PI / 180.f;
    map->cam->phi = 0.f * M_PI / 180.f;
   
    map->cam->calculateViewport();
    map->cam->debugPrintCorners();

    // A yellow sphere
    map->appendSphere({5.f, 0.f, 1.f}, 0.3f, {0.8f, 0.82f, 0.2f}, 0.4f); 
    map->appendSphere({5.2f, 0.f, -0.2f}, 0.4f, {0.25f, 0.82f, 0.82f}, 0.5f); 
    map->appendSphere({2.f, -0.8f, -1.4f}, 0.4f, {0.82f, 0.25f, 0.82f}, 0.1f); 
    

    img = newImage(initialWidth, initialHeight);
    clearImage(img);

    GLWindow window(initialWidth, initialHeight);
    // castRays(&map, img);
    window.mainLoop(mainLoop);

    // writeBMP(img, "/tmp/test.bmp");

}
