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

#define FRAMETIME

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
    if (window.state.fbWidth != window.state.w || window.state.fbHeight != window.state.h) {
        window.state.w = float(window.state.fbWidth) * window.state.scale;
        window.state.h = float(window.state.fbHeight) * window.state.scale;
        map->cam->setDimensions(window.state.w, window.state.h);
        resizeImage(img, window.state.w, window.state.h);
    }
    if (map->cam->phi != window.state.mouse.phi || map->cam->theta != window.state.mouse.theta) {
        map->cam->rotateRad(window.state.mouse.theta, window.state.mouse.phi);
    }
    clearImage(img);
    map->castRays(img);
    std::fprintf(stderr, "Rays casted\n");

    return img;
}

int main(void) {
    map = new WorldMap("maps/3sphere.map");

    map->cam = new Camera(window.state.w, window.state.h, initialFOV, {0.f, 0.f, 0.f});
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
