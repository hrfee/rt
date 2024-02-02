#include "../map/map.hpp"
#include "../img/img.hpp"
#include "../render_gl/gl.hpp"
#include "../cam/cam.hpp"
#include <math.h>
#include <cstdio>
#include <getopt.h>

#define INIT_W 1280 
#define INIT_H 720
#define INIT_SCALE_FACTOR .2f
#define INIT_FOV 60

#define WINDOW_TITLE "COMP3931 Individual Project - rt"

// Uncomment to get printouts of frametime & fps. Forces re-rendering every frame.
// #define FRAMETIME

namespace {
    WorldMap *map;
    Image *img;
    GLWindow *window;
}

Image *mainLoop(bool renderOnChange, bool renderNow) {
#ifdef FRAMETIME
    double frameTime = glfwGetTime();
    std::fprintf(stderr, "Frame time: %dms (%.2f FPS)\n", int((frameTime-window->state.lastFrameTime)*1000.f), 1.f/(frameTime-window->state.lastFrameTime));
    window->state.lastFrameTime = glfwGetTime();
#endif
#ifndef FRAMETIME
    bool change = renderNow;
#endif
    if (renderOnChange || renderNow) {
        if (window->state.w != window->state.prevW || window->state.h != window->state.prevH) {
            map->cam->setDimensions(window->state.w, window->state.h);
            resizeImage(img, window->state.w, window->state.h);
            window->state.prevW = window->state.w;
            window->state.prevH = window->state.h;
            window->state.prevScale = window->state.scale;
#ifndef FRAMETIME
            change = true;
#endif
        }
        if (window->state.fovDeg != window->state.prevFovDeg) {
            map->cam->setFOV(window->state.fovDeg * M_PI / 180.f);
            window->state.prevFovDeg = window->state.fovDeg;
#ifndef FRAMETIME
            change = true;
#endif
        }
    }
    if (map->cam->phi != window->state.mouse.phi || map->cam->theta != window->state.mouse.theta) {
        map->cam->rotateRad(window->state.mouse.theta, window->state.mouse.phi);
#ifndef FRAMETIME
        change = true;
#endif
    }
#ifndef FRAMETIME
    if (change) {
#endif
        clearImage(img);
        window->state.lastRenderTime = glfwGetTime();
        map->castRays(img);
        window->state.lastRenderTime = glfwGetTime() - window->state.lastRenderTime;
        window->state.lastRenderW = window->state.w;
        window->state.lastRenderH = window->state.h;
#ifndef FRAMETIME
    }
#endif
    return img;
}

int main(int argc, char **argv) {
    int windowWidth = INIT_W;
    int windowHeight = INIT_H;
    float windowScaleFactor = INIT_SCALE_FACTOR;
    float cameraFOV = float(INIT_FOV)*M_PI / 180.f;

    int flag;
    while ((flag = getopt(argc, argv, "hW:H:s:f:")) != -1) {
        switch (flag) {
            case 'W': {
                windowWidth = std::stoi(std::string(optarg));
            }; break;
            case 'H': {
                windowHeight = std::stoi(std::string(optarg));
            }; break;
            case 's': {
                windowScaleFactor = std::stof(std::string(optarg));
            }; break;
            case 'f': {
                cameraFOV = float(std::stoi(std::string(optarg))) * M_PI / 180.f;
            }; break;
            case 'h': {
                std::fprintf(stderr, "%s\n%s -W <window width> -H <window height> -s <render scale factor> -f <fov (degrees)>\n", WINDOW_TITLE, argv[0]);
                return 0;
            }; break;
            case '?': {
                if (optopt == 'W' || optopt == 'H') {
                    std::fprintf(stderr, "-%c requires an integer argument.\n", optopt);
                } else if (optopt == 's') {
                    std::fprintf(stderr, "-%c requires a number scale value.\n", optopt);
                }
                return 1;
            };
        }
    }

    window = new GLWindow(windowWidth, windowHeight, windowScaleFactor, WINDOW_TITLE);
    map = new WorldMap("maps/3sphere.map");

    map->cam = new Camera(window->state.w, window->state.h, cameraFOV, {0.f, -0.5f, 0.f});
    window->state.fovDeg = cameraFOV * 180.f / M_PI;
    window->state.prevFovDeg = window->state.fovDeg;
    window->state.mouse.phi = 0.f;
    window->state.mouse.theta = 0.f;
    map->cam->rotateRad(window->state.mouse.theta, window->state.mouse.phi);
   
    // map->cam->debugPrintCorners();

    img = newImage(window->state.w, window->state.h);
    clearImage(img);

    window->mainLoop(mainLoop);
    delete window;
    delete map;
    closeImage(img);
}
