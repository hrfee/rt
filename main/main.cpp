#include "map.hpp"
#include "img.hpp"
#include "gl.hpp"
#include "cam.hpp"
#include <math.h>
#include <cstdio>
#include <getopt.h>
#include <thread>

#define INIT_W 1280 
#define INIT_H 720
#define INIT_SCALE_FACTOR .2f
#define INIT_FOV 60

#define WINDOW_TITLE "COMP3931 Individual Project - rt"
#define MAP_PATH "maps/checker.map"
// Uncomment to get printouts of frametime & fps. Forces re-rendering every frame.
// #define FRAMETIME

namespace {
    WorldMap *map;
    Image *img;
    GLWindow *window;
}

Image *mainLoop(RenderConfig *rc) {
#ifdef FRAMETIME
    double frameTime = glfwGetTime();
    std::fprintf(stderr, "Frame time: %dms (%.2f FPS)\n", int((frameTime-window->state.lastFrameTime)*1000.f), 1.f/(frameTime-window->state.lastFrameTime));
    window->state.lastFrameTime = glfwGetTime();
#endif
#ifndef FRAMETIME
    bool change = rc->renderNow;
#endif
    if (window->state.reloadMap) {
        map->loadFile(window->state.mapPath.c_str());
        window->state.reloadMap = false;
        window->state.useOptimizedMap = false;
        change = true;
    }

    if (window->state.useOptimizedMap) {

        if (map->optimizedObj == NULL || (window->state.hierarchyDepth != map->optimizeLevel && change) || window->state.hierarchySplitterIndex != map->splitterIndex) {
            map->splitterIndex = window->state.hierarchySplitterIndex;
            map->optimizeMap(window->state.hierarchyDepth, map->splitterIndex);
            window->state.optimizedMap = map->optimizedObj;
        }
        map->obj = map->optimizedObj;
    } else {
        map->obj = &(map->unoptimizedObj);
    }

    if (rc->renderOnChange || rc->renderNow) {
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
    if (window->state.mouse.moveForward != 0.f) {
        map->cam->setPosition(map->cam->position + (window->state.mouse.moveForward * map->cam->viewportNormal));
        rc->manualPosition = map->cam->position;
        change = true;
    }
    if (window->state.mouse.moveSideways != 0.f) {
        map->cam->setPosition(map->cam->position + (window->state.mouse.moveSideways * map->cam->viewportParallel));
        rc->manualPosition = map->cam->position;
        change = true;
    }
#ifndef FRAMETIME
    if (change && !map->currentlyRendering) {
#endif
        clearImage(img);
        // map->castRays(img, rc, glfwGetTime);
        window->state.currentlyRendering = true;
        std::thread cast(&WorldMap::castRays, map, img, rc, glfwGetTime);
        cast.detach(); // Keep running when this loop finishes and cast goes out of scope
        /*cast.join();
        window->state.lastRenderTime = map->lastRenderTime;
        window->state.lastRenderW = window->state.w;
        window->state.lastRenderH = window->state.h;*/
#ifndef FRAMETIME
    } else if (!map->currentlyRendering) {
        window->state.currentlyRendering = false;
        window->state.lastRenderTime = map->lastRenderTime;
        window->state.lastRenderW = window->state.w;
        window->state.lastRenderH = window->state.h;
    } else {
        window->state.lastRenderTime = glfwGetTime() - map->lastRenderTime;
    }

#endif
    return img;
}

int main(int argc, char **argv) {
    int windowWidth = INIT_W;
    int windowHeight = INIT_H;
    float windowScaleFactor = INIT_SCALE_FACTOR;
    float cameraFOV = float(INIT_FOV)*M_PI / 180.f;
    std::string mapPath = MAP_PATH;

    int flag;
    while ((flag = getopt(argc, argv, "hW:H:s:f:m:")) != -1) {
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
            case 'm': {
                mapPath = std::string(optarg);
            }; break;
            case 'h': {
                std::fprintf(stderr, "%s\n%s -W <window width> -H <window height> -s <render scale factor> -f <fov (degrees)> -m <map path>\n", WINDOW_TITLE, argv[0]);
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
    map = new WorldMap(mapPath.c_str());
    window->state.mapPath = mapPath.c_str();

    // map->o = splitKD(&(map->o), 10);

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
