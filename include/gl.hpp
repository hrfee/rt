#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "img.hpp"
#include "map.hpp"
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
    double lastFrameTime;
    double lastRenderTime;
    int lastRenderW, lastRenderH;
    float fovDeg, prevFovDeg;
    RenderConfig rc;
    std::string filePath;
    std::string mapPath;
    bool reloadMap;
    bool useOptimizedMap;
    bool renderOptimizedHierarchy;
    Container *optimizedMap;
    double lastOptimizeTime;
    int hierarchySplitterIndex;
    int hierarchyDepth;
    int hierarchyExtraParam;
    bool useBVH;
    bool staleHierarchyConfig;
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
    } mouse;
    bool currentlyRendering;
    bool currentlyOptimizing;
    std::stringstream csvStats;
    bool dumpToCsv;
    bool csvDirty;
};

struct GLWindowUI {
    ImGuiContext *ctx;
    bool hide;
    bool saveToTGA;
    int renderMode;
};

class GLWindow {
    public:
        GLFWwindow* window;
        GLWindow(int w, int h, float scale, const char *windowTitle);
        ~GLWindow();
        void mainLoop(Image* (*func)(RenderConfig *c));
        void draw(Image *img);
        GLWindowState state;
        void reloadTexture();
        void reloadScaledResolution();
        void loadRequestedWindowResolution();
        std::string frameInfo();
        std::string hierarchyInfo();
        std::string threadInfo();
        void generateFrameVertices();
        void toggleUI() { ui.hide = !ui.hide; };
        void hideUI() { ui.hide = true; };
        void showUI() { ui.hide = false; };
        GLWindowUI ui;
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
};

void resizeWindowCallback(GLFWwindow *, int, int);
void mouseCallback(GLFWwindow *, double, double);
void keyCallback(GLFWwindow *, int, int, int, int);

void showKeyboardHelp();

#endif
