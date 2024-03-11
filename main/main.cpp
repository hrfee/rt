#include "map.hpp"
#include "img.hpp"
#include "gl.hpp"
#include "cam.hpp"
#include "hierarchy.hpp"
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

// RENDER STATS
// mapName,numTris,numSpheres,numLights,rayBounces,renderMode,camPhi,camTheta,camFOVDeg,camPos(spaceseparated),renderTime(ms),width,height,nthreads,bvhMethod,maxDepth,buildTime(ms)
std::string csvStats(GLWindow *window, WorldMap *map, bool header = false) {
    std::ostringstream out;
    if (header) {
        out << "Map Name,# Tris,# Spheres,# Lights,Max Ray bounces,Render mode,Phi (l/r) (radians),Theta (u/d) (radians),Field of View (°),Position (x y z),Render time (ms),Width (px),Height (px),# Threads,Acceleration Method,Max splitting depth,Structure build time (ms)" << ",";
        out << std::endl;
    }
    // Map & Cam info
    out << map->mapStats.name << "," << map->mapStats.tris << "," << map->mapStats.spheres << "," << map->mapStats.lights << ",";
    out << window->state.rc.maxBounce << ",";
    out << modes[window->ui.renderMode] << ",";
    out << window->state.mouse.phi << "," << window->state.mouse.theta << "," << window->state.fovDeg << ",";
    out << window->state.rc.manualPosition.x << " " << window->state.rc.manualPosition.y << " " << window->state.rc.manualPosition.z << ",";
    // Render info
    out << int(window->state.lastRenderTime * 1000.f) << ",";
    out << window->state.lastRenderW << "," << window->state.lastRenderH << ",";
    out << window->state.rc.nthreads << ",";
    // Hierarchy Info
    if (window->state.staleHierarchyConfig || !(window->state.useOptimizedMap)) {
        out << ",,,";
    } else {
        out << splitters[window->state.hierarchySplitterIndex] << ",";
        out << window->state.hierarchyDepth << ",";
        out << int(window->state.lastOptimizeTime * 1000.f) << ",";
    }
    
    out << std::endl;
    return out.str();
}

Image *mainLoop(RenderConfig *rc) {
    bool change = rc->renderNow;
    if (window->state.reloadMap) {
        map->loadFile(window->state.mapPath.c_str());
        window->state.reloadMap = false;
        window->state.useOptimizedMap = false;
        window->state.staleHierarchyConfig = true;
        change = true;
        window->state.camPresets = &(map->camPresets);
        window->state.camPresetNames = map->camPresetNames;
        window->state.camPresetCount = map->camPresets.size();
    }

    if (window->state.useOptimizedMap) {

        bool hierarchyChanged = 
            (window->state.hierarchyDepth != map->optimizeLevel ||
            window->state.hierarchySplitterIndex != map->splitterIndex ||
            window->state.useBVH != map->bvh ||
            window->state.hierarchyExtraParam != map->splitterParam ||
            window->state.staleHierarchyConfig);

        if (hierarchyChanged && !change) { window->state.staleHierarchyConfig = true; }

        if (
            // map->optimizedObj == NULL ||
            (change && hierarchyChanged && !map->currentlyOptimizing)
        ) {
            map->splitterIndex = window->state.hierarchySplitterIndex;
            map->bvh = window->state.useBVH;
            map->splitterParam = window->state.hierarchyExtraParam;
            window->state.currentlyOptimizing = true;
            std::thread opt(&WorldMap::optimizeMap, map, glfwGetTime, window->state.hierarchyDepth, map->splitterIndex);
            opt.detach();
            window->state.staleHierarchyConfig = false;
        } else if (!map->currentlyOptimizing) {
            window->state.lastOptimizeTime = map->lastOptimizeTime;
            window->state.optimizedMap = map->optimizedObj;
            window->state.currentlyOptimizing = false;
        } else {
            window->state.lastOptimizeTime = glfwGetTime() - map->lastOptimizeTime;
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

    if (change && !map->currentlyRendering && !window->state.currentlyOptimizing) {
        clearImage(img);
        // map->castRays(img, rc, glfwGetTime);
        window->state.csvDirty = true;
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
        if (window->state.dumpToCsv && window->state.csvDirty) {
            window->state.csvStats << csvStats(window, map, window->state.csvStats.tellp() == 0 ? true : false);
            window->state.csvDirty = false;
        }
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
