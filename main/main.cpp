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

namespace {
    WorldMap *map;
    Image *img;
    GLWindow *window;
}

Image *mainLoop(RenderConfig *rc) {
    bool change = rc->renderNow;
    if (window->state.reloadMap) {
        map->loadFile(window->state.mapPath.c_str());
        window->state.reloadMap = false;
        window->state.useOptimizedMap = false;
        change = true;
        window->state.camPresets = &(map->camPresets);
        if (map->camPresetNames != NULL && map->camPresets.size() != 0) {
            window->state.camPresetNames = map->camPresetNames;
            window->state.camPresetCount = map->camPresets.size();
        }
    }

    if (window->state.useOptimizedMap) {

        bool hierarchyChanged = 
            (window->state.hierarchyDepth != map->optimizeLevel ||
            window->state.hierarchySplitterIndex != map->splitterIndex ||
            window->state.useBVH != map->bvh ||
            window->state.hierarchyExtraParam != map->splitterParam);

        if (hierarchyChanged && !change) { window->state.staleHierarchyConfig = true; }

        if (
            map->optimizedObj == NULL ||
            (change && hierarchyChanged)
        ) {
            map->splitterIndex = window->state.hierarchySplitterIndex;
            map->bvh = window->state.useBVH;
            map->splitterParam = window->state.hierarchyExtraParam;
            map->optimizeMap(glfwGetTime, window->state.hierarchyDepth, map->splitterIndex);
            window->state.lastOptimizeTime = map->lastOptimizeTime;
            window->state.optimizedMap = map->optimizedObj;
            window->state.staleHierarchyConfig = false;
        }
        map->obj = map->optimizedObj;
    } else {
        map->obj = &(map->unoptimizedObj);
    }

    // If the requested render res has changed
    if (window->state.rDim.dirty()) {
        map->cam->setDimensions(window->state.rDim.w, window->state.rDim.h);
        resizeImage(img, window->state.rDim.w, window->state.rDim.h);
        window->state.texDim.w = window->state.rDim.w;
        window->state.texDim.h = window->state.rDim.h;
        clearImage(img);
        if (rc->renderOnChange) change = true;
        window->state.rDim.update();
    }
    // std::printf("current values: %dx%d, %dx%d, %dx%d, %dx%d, %dx%d\n", window->state.w, window->state.h, window->state.fbWidth, window->state.fbHeight, window->state.prevW, window->state.prevH, window->state.prevFbWidth, window->state.prevFbHeight, map->cam->w, map->cam->h);
    if (rc->renderOnChange || rc->renderNow) {
        if (window->state.fovDeg != window->state.prevFovDeg) {
            map->cam->setFOV(window->state.fovDeg * M_PI / 180.f);
            window->state.prevFovDeg = window->state.fovDeg;
            change = true;
        }
    }
    if (map->cam->phi != window->state.mouse.phi || map->cam->theta != window->state.mouse.theta) {
        map->cam->rotateRad(window->state.mouse.theta, window->state.mouse.phi);
        change = true;
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

    if (window->state.maxThreadCount == -1) {
        window->state.maxThreadCount = std::thread::hardware_concurrency();
    }
    if (window->state.threadCount == -1) {
        window->state.threadCount = std::thread::hardware_concurrency();
    }

    if (change && !map->currentlyRendering) {
        clearImage(img);
        // map->castRays(img, rc, glfwGetTime);
        window->state.currentlyRendering = true;
        if (window->state.mouse.enabled) {
            map->castRays(img, rc, glfwGetTime, window->state.threadCount);
        } else {
            std::thread cast(&WorldMap::castRays, map, img, rc, glfwGetTime, window->state.threadCount);
            cast.detach(); // Keep running when this loop finishes and cast goes out of scope
        }
    } else if (!map->currentlyRendering) {
        window->state.currentlyRendering = false;
        window->state.lastRenderTime = map->lastRenderTime;
        window->state.lastRenderW = window->state.rDim.w;
        window->state.lastRenderH = window->state.rDim.h;
    } else {
        window->state.lastRenderTime = glfwGetTime() - map->lastRenderTime;
    }

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
    if (map->camPresetNames != NULL) {
        window->state.camPresets = &(map->camPresets);
        window->state.camPresetNames = map->camPresetNames;
        window->state.camPresetCount = map->camPresets.size();
    }

    // map->o = splitKD(&(map->o), 10);

    map->cam = new Camera(window->state.rDim.w, window->state.rDim.h, cameraFOV, {0.f, -0.5f, 0.f});
    window->state.fovDeg = cameraFOV * 180.f / M_PI;
    window->state.prevFovDeg = window->state.fovDeg;
    window->state.mouse.phi = 0.f;
    window->state.mouse.theta = 0.f;
    map->cam->rotateRad(window->state.mouse.theta, window->state.mouse.phi);
   
    // map->cam->debugPrintCorners();

    img = newImage(window->state.rDim.w, window->state.rDim.h);
    clearImage(img);

    window->mainLoop(mainLoop);
    delete window;
    delete map;
    closeImage(img);
}
