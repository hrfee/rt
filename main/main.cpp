#include "map.hpp"
#include "img.hpp"
#include "gl.hpp"
#include "cam.hpp"
#include "accel.hpp"
#include "shape.hpp"
#include <cstdio>
#include <getopt.h>
#include <thread>

#define INIT_W 1280 
#define INIT_H 720
#define INIT_SCALE_FACTOR .2f
#define INIT_FOV 60

#define WINDOW_TITLE "COMP3931 Individual Project - rt"
#define MAP_PATH "maps/scene.map"

namespace {
    WorldMap *map;
    Image *img;
    SubImage *aaOffsetMap;
    GLWindow *window;
}

// RENDER STATS
// mapName,numTris,numSpheres,numLights,rayBounces,renderMode,camPhi,camTheta,camFOVDeg,camPos(spaceseparated),renderTime(ms),width,height,nthreads,bvhMethod,maxDepth,buildTime(ms)
std::string csvStats(GLWindow *window, WorldMap *map, bool header = false) {
    std::ostringstream out;
    if (header) {
        out << "Map Name,# Tris,# Planes,# Spheres,# AABs,# Lights,Max Ray bounces,Render mode,Phi (l/r) (radians),Theta (u/d) (radians),Field of View (°),Position (x y z),Render time (ms),Width (px),Height (px),# Threads,Acceleration Method,Max splitting depth,Structure build time (ms),Accel Int Param,Accel Float Param," << ",";
        out << std::endl;
    }
    // Map & Cam info
    out << map->mapStats.name << "," << map->mapStats.tris << "," << map->mapStats.planes << "," << map->mapStats.spheres << "," << map->mapStats.aabs << "," << map->mapStats.lights << ",";
    out << window->state.rc.maxBounce << ",";
    out << modes[window->ui.renderMode] << ",";
    out << window->state.mouse.phi << "," << window->state.mouse.theta << "," << window->state.fovDeg << ",";
    out << window->state.rc.manualPosition.x << " " << window->state.rc.manualPosition.y << " " << window->state.rc.manualPosition.z << ",";
    // Render info
    out << int(window->state.lastRenderTime * 1000.f) << ",";
    out << window->state.lastRenderW << "," << window->state.lastRenderH << ",";
    out << window->state.rc.nthreads << ",";
    // Hierarchy Info
    if (window->state.staleAccelConfig || !(window->state.useOptimizedMap)) {
        out << ",,,,,";
    } else {
        out << accelerators[window->state.accelIndex] << ",";
        out << window->state.accelDepth << ",";
        out << int(window->state.lastOptimizeTime * 1000.f) << ",";
        out << map->accelParam << ",";
        out << map->accelFloatParam << ",";
    }

    
    out << std::endl;
    return out.str();
}


Image *mainLoop(RenderConfig *rc) {
    bool change = (rc->renderOnChange && rc->change) || rc->renderNow;
    if (window->state.reloadMap && !map->currentlyLoading) {
        if (!window->state.currentlyLoading) {
            window->state.currentlyLoading = true;
            // map->loadFile(window->state.mapPath.c_str());
            window->state.camPresetCount = 0;
            window->state.camPresetNames = NULL;
            window->state.rc.baseBrightness = -1.f;
            window->state.rc.globalShininess = -1.f;
            std::thread load(&WorldMap::loadFile, map, window->state.mapPath.c_str(), glfwGetTime);
            load.detach();
        } else {
            window->state.reloadMap = false;
            window->state.useOptimizedMap = false;
            window->state.staleAccelConfig = true;
            change = true;
            window->state.camPresets = &(map->camPresets);
            window->state.camPresetNames = map->camPresetNames;
            window->state.camPresetCount = map->camPresets.size();
            window->state.objectPtrs = map->objectPtrs;
            window->state.objectCount = map->objectCount;
            window->state.plCount = map->mapStats.lights;
            window->state.objectNames = map->objectNames;
            window->state.currentlyLoading = false;
        }
    } else if (map->currentlyLoading) {
        window->state.lastLoadTime = glfwGetTime() - map->lastLoadTime;
    }

    if (window->state.useOptimizedMap) {
        bool hierarchyChanged = 
            (window->state.accelDepth != map->optimizeLevel ||
            window->state.accelIndex != map->accelIndex ||
            window->state.useBVH != map->bvh ||
            window->state.accelParam != map->accelParam ||
            window->state.accelFloatParam != map->accelFloatParam ||
            window->state.staleAccelConfig);

        if (hierarchyChanged && !change) { window->state.staleAccelConfig = true; }

        if (
            // map->optimizedObj == NULL ||
            (change && hierarchyChanged && !map->currentlyOptimizing)
        ) {
            map->accelIndex = window->state.accelIndex;
            map->bvh = window->state.useBVH;
            map->accelParam = window->state.accelParam;
            map->accelFloatParam = window->state.accelFloatParam;
            window->state.currentlyOptimizing = true;
            std::thread opt(&WorldMap::optimizeMap, map, glfwGetTime, window->state.accelDepth, map->accelIndex);
            opt.detach();
            window->state.staleAccelConfig = false;
        } else if (!map->currentlyOptimizing) {
            window->state.lastOptimizeTime = map->lastOptimizeTime;
            window->state.optimizedMap = map->optimizedObj;
            window->state.objectPtrs = map->objectPtrs;
            window->state.objectNames = map->objectNames;
            window->state.currentlyOptimizing = false;
        } else {
            window->state.lastOptimizeTime = glfwGetTime() - map->lastOptimizeTime;
        }
        map->obj = map->optimizedObj;
    } else {
        if (map->obj == map->optimizedObj) {
            map->genObjectList(&(map->unoptimizedObj));
            window->state.objectPtrs = map->objectPtrs;
            window->state.objectNames = map->objectNames;
        }
        map->obj = &(map->unoptimizedObj);
    }

    if (window->state.recalcUVs) {
        Shape *sh = window->getShapePointer();
        if (sh != NULL) {
            Triangle *t = dynamic_cast<Triangle*>(sh);
            AAB *b = dynamic_cast<AAB*>(sh);
            if (t != nullptr) map->dec.recalculateTriUVs(t);
            else if (b != nullptr) map->dec.recalculateAABUVs(b);
        }
        window->state.recalcUVs = false;
    }

    // If the requested render res has changed
    if (window->state.rDim.dirty()) {
        map->cam->setDimensions(window->state.rDim.w, window->state.rDim.h);
        img->resize(window->state.rDim.w, window->state.rDim.h);
        window->state.texDim.w = window->state.rDim.w;
        window->state.texDim.h = window->state.rDim.h;
        img->clear();
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

    double time = glfwGetTime();
    double delta = time - window->state.renderedAtTime;

    if (map->cam->phi != window->state.mouse.phi || map->cam->theta != window->state.mouse.theta) {
        map->cam->rotateRad(window->state.mouse.theta, window->state.mouse.phi);
        change = true;
    }
    if (window->state.mouse.moveForward != 0.f) {
        map->cam->setPosition(map->cam->position + (window->state.mouse.moveForward * window->state.mouse.speedMultiplier * map->mapStats.moveSpeedMultiplier * delta * map->cam->viewportNormal));
        rc->manualPosition = map->cam->position;
        change = true;
    }
    if (window->state.mouse.moveSideways != 0.f) {
        map->cam->setPosition(map->cam->position + (window->state.mouse.moveSideways * window->state.mouse.speedMultiplier * map->mapStats.moveSpeedMultiplier * delta * map->cam->viewportParallel));
        rc->manualPosition = map->cam->position;
        change = true;
    }

    window->state.renderedAtTime = time;

    if (window->state.maxThreadCount == -1) {
        window->state.maxThreadCount = std::thread::hardware_concurrency();
    }
    if (window->state.threadCount == -1) {
        window->state.threadCount = std::thread::hardware_concurrency();
    }

    if (change && !map->currentlyRendering && !window->state.currentlyOptimizing && !map->currentlyLoading) {
        img->clear();
        // map->castRays(img, rc, glfwGetTime);
        window->state.csvDirty = true;
        window->state.currentlyRendering = true;
        if (window->state.mouse.enabled || rc->renderOnChange) {
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

    if (map->aaOffsetImageDirty) {
        aaOffsetMap->dirty = true;
        map->aaOffsetImageDirty = false;
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
    if (map->camPresetNames != NULL) {
        window->state.camPresets = &(map->camPresets);
        window->state.camPresetNames = map->camPresetNames;
        window->state.camPresetCount = map->camPresets.size();
    }
    window->state.objectPtrs = map->objectPtrs;
    window->state.objectCount = map->objectCount;
    window->state.plCount = map->mapStats.lights;
    window->state.objectNames = map->objectNames;
    window->state.materials = &(map->materials);
    window->state.tex = &(map->tex);
    window->state.norms = &(map->norms);
    window->state.refs = &(map->refs);

    // map->o = splitKD(&(map->o), 10);

    map->cam = new Camera(window->state.rDim.w, window->state.rDim.h, cameraFOV, {0.f, -0.5f, 0.f});
    window->state.fovDeg = cameraFOV * 180.f / M_PI;
    window->state.prevFovDeg = window->state.fovDeg;
    window->state.mouse.phi = 0.f;
    window->state.mouse.theta = 0.f;
    map->cam->rotateRad(window->state.mouse.theta, window->state.mouse.phi);
   
    // map->cam->debugPrintCorners();

    img = new Image(window->state.rDim.w, window->state.rDim.h);
    img->clear();

    aaOffsetMap = new SubImage(new Image(102, 102)); // 100^2 plus space for a border
    map->aaOffsetImage = aaOffsetMap->img;
    window->images.emplace_back(aaOffsetMap);

    window->state.mapStats = &(map->mapStats);
    window->state.mapPath = map->mapStats.name;

    window->mainLoop(mainLoop);
    delete window;
    delete map;
    delete img;
}
