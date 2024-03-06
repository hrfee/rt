#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "img.hpp"
#include "map.hpp"
#include <string>

struct GLWindowState {
    // The render resolution
    int w, h, prevW, prevH;
    // The window resolution
    int fbWidth, fbHeight, prevFbWidth, prevFbHeight;
    int requestedFbW, requestedFbH;
    float scale, prevScale;
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
};

struct GLWindowUI {
    ImGuiContext *ctx;
    bool widthApply, heightApply, saveToTGA;
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
        std::string frameInfo();
        std::string hierarchyInfo();
    private:
        const char *title;
        std::string vertString, fragString;
        GLuint tex, vert, frag, prog, vao;
        void loadShader(GLuint *s, GLenum shaderType, char const *fname);
        GLWindowUI ui;
        void loadUI();
        void showUI();
        void renderTree(Container *c, int tabIndex = 0);
};

void resizeWindowCallback(GLFWwindow *, int, int);
void mouseCallback(GLFWwindow *, double, double);
void keyCallback(GLFWwindow *, int, int, int, int);

void showKeyboardHelp();

#endif
