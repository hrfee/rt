#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "img.hpp"
#include "map.hpp"
#include "shape.hpp"
#include <string>

struct Dimensions {
    int w, h;
    int pW, pH;
    constexpr bool dirty() noexcept {
        return (w != pW || h != pH);
    };
    void update() {
        pW = w;
        pH = h;
    };
    void scaleFrom(Dimensions *base, float s) {
        w = int(float(base->w) * s);
        h = int(float(base->h) * s);
    }
};

struct GLWindowState {
    // The render resolution
    Dimensions vpDim, rDim, fbDim, texDim;
    float scale;
    bool scaleDirty;
    double renderedAtTime;
    double lastRenderTime;
    int lastRenderW, lastRenderH;
    float fovDeg, prevFovDeg;
    RenderConfig rc;
    std::string filePath;
    std::string mapPath;
    MapStats *mapStats;
    bool reloadMap;
    bool useOptimizedMap;
    bool renderOptimizedHierarchy;
    Container *optimizedMap;
    double lastOptimizeTime;
    double lastLoadTime;
    int accelIndex;
    int accelDepth;
    int accelParam;
    float accelFloatParam;
    bool useBVH;
    bool staleAccelConfig;
    int threadCount;
    int maxThreadCount;
    std::vector<CamPreset> *camPresets;
    const char** camPresetNames;
    int camPresetIndex;
    int camPresetCount;
    struct Mouse {
        bool enabled;
        float sensitivity;
        float prevX, prevY;
        float phi, theta;
        float moveForward;
        float moveSideways;
        float speedMultiplier;
    } mouse;
    bool currentlyRendering;
    bool currentlyOptimizing;
    bool currentlyLoading;
    std::stringstream csvStats;
    bool dumpToCsv;
    bool csvDirty;
    char **objectNames;
    int objectCount;
    int plCount;
    Shape **objectPtrs;
    int objectIndex;
    MaterialStore *materials;
    int materialIndex;
    bool recalcUVs;
};

struct GLWindowUI {
    ImGuiContext *ctx;
    bool hide;
    bool saveToTGA;
    int renderMode;
};

struct SubImage {
    Image *img;
    bool sizeChanged;
    bool dirty;
    GLuint tex;
    SubImage(Image *image) {
        img = image;
        dirty = true;
        sizeChanged = true;
    }
    void regen();
    void upload();
    void show();
};

class GLWindow {
    public:
        GLFWwindow* window;
        GLWindow(int w, int h, float scale, const char *windowTitle);
        ~GLWindow();
        void mainLoop(Image* (*func)(RenderConfig *c));
        void draw(Image *img);
        GLWindowState state;
        void reloadRenderTexture();
        void reloadScaledResolution();
        void loadRequestedWindowResolution();
        std::string frameInfo();
        std::string acceleratorInfo();
        void threadInfo();
        // std::string threadInfo();
        std::string mapLoadInfo();
        void generateFrameVertices();
        void toggleUI() { ui.hide = !ui.hide; };
        void hideUI() { ui.hide = true; };
        void showUI() { ui.hide = false; };
        GLWindowUI ui;
        std::vector<SubImage*> images;
        bool shouldntBeDoingAnything();
        Shape *getShapePointer();
        Material *getMaterialPointer();
    private:
        const char *title;
        std::string vertString, fragString;
        GLuint tex, vert, frag, prog, vao;
        void loadShader(GLuint *s, GLenum shaderType, char const *fname);
        void loadUI();
        void addUI();
        void renderTree(Container *c, int tabIndex = 0);
        GLfloat glFrameVertices[8];
        void disable();
        void enable();

        bool vl(bool t);

        void showShapeEditor();
        void showMaterialEditor(Material *m);
};

void resizeWindowCallback(GLFWwindow *, int, int);
void mouseCallback(GLFWwindow *, double, double);
void keyCallback(GLFWwindow *, int, int, int, int);

void showKeyboardHelp();

#endif
