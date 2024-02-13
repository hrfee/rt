#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "../img/img.hpp"
#include "../map/map.hpp"
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
    struct Mouse {
        bool enabled;
        float sensitivity;
        float prevX, prevY;
        float phi, theta;
        float moveForward;
        float moveSideways;
    } mouse;
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
    private:
        const char *title;
        std::string vertString, fragString;
        GLuint tex, vert, frag, prog, vao;
        void loadShader(GLuint *s, GLenum shaderType, char const *fname);
        GLWindowUI ui;
        void loadUI();
        void showUI();
};

void resizeWindowCallback(GLFWwindow *, int, int);
void mouseCallback(GLFWwindow *, double, double);
void keyCallback(GLFWwindow *, int, int, int, int);

void showKeyboardHelp();

#endif
