#include "../map/map.hpp"
#include "../img/img.hpp"
#include "../render_bmp/bmp.hpp"
#include "../render_gl/gl.hpp"
#include "../cam/cam.hpp"
#include <math.h>
#include <cstdio>

#define TEST_W 1920 
#define TEST_H 1080
#define TEST_RES_MULTIPLIER .2f
#define TEST_FOV 60

// Uncomment to get printouts of frametime & fps. Forces re-rendering every frame.
// #define FRAMETIME

namespace {
    constexpr int initialWidth = int(float(TEST_W)*TEST_RES_MULTIPLIER);
    constexpr int initialHeight = int(float(TEST_H)*TEST_RES_MULTIPLIER);
    constexpr float initialFOV = float(TEST_FOV)*M_PI / 180.f;
    WorldMap *map;
    Image *img;
    GLWindow window(TEST_W, TEST_H, TEST_RES_MULTIPLIER);
}

Image *mainLoop() {
#ifdef FRAMETIME
    double frameTime = glfwGetTime();
    std::fprintf(stderr, "Frame time: %dms (%.2f FPS)\n", int((frameTime-window.state.lastFrameTime)*1000.f), 1.f/(frameTime-window.state.lastFrameTime));
    window.state.lastFrameTime = glfwGetTime();
#endif
#ifndef FRAMETIME
    bool change = false;
#endif
    if (window.state.w != window.state.prevW || window.state.h != window.state.prevH) {
        map->cam->setDimensions(window.state.w, window.state.h);
        resizeImage(img, window.state.w, window.state.h);
        window.state.prevW = window.state.w;
        window.state.prevH = window.state.h;
#ifndef FRAMETIME
        change = true;
#endif
    }
    if (map->cam->phi != window.state.mouse.phi || map->cam->theta != window.state.mouse.theta) {
        map->cam->rotateRad(window.state.mouse.theta, window.state.mouse.phi);
#ifndef FRAMETIME
        change = true;
#endif

    }
#ifndef FRAMETIME
    if (change) {
#endif
        clearImage(img);
        map->castRays(img);
        std::fprintf(stderr, "Rays casted\n");
#ifndef FRAMETIME
    }
#endif
    return img;
}

int main(void) {

    /*if (pointInTriangle({7.27, -0.21}, {8, 0}, {7, -1}, {6, 5})) {
        std::printf("i work!\n");
    } else {
        std::printf("i don't work!\n");
    }
    return 0;*/

    map = new WorldMap("maps/3sphere.map");

    map->cam = new Camera(window.state.w, window.state.h, initialFOV, {0.f, -0.5f, 0.f});
    window.state.mouse.phi = 0.f;
    window.state.mouse.theta = 0.f;
    map->cam->rotateRad(window.state.mouse.theta, window.state.mouse.phi);
   
    map->cam->debugPrintCorners();

   


    img = newImage(window.state.w, window.state.h);
    clearImage(img);

    // castRays(&map, img);
    window.mainLoop(mainLoop);

    // writeBMP(img, "/tmp/test.bmp");

}
