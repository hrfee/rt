#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "../img/img.hpp"
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
    bool renderOnChange;
    std::string filePath;
    struct Mouse {
        bool enabled;
        float sensitivity;
        float prevX, prevY;
        float phi, theta;
    } mouse;
};

struct GLWindowUI {
    ImGuiContext *ctx;
    ImGuiIO *io;
};

class GLWindow {
    public:
        GLFWwindow* window;
        GLWindow(int w, int h, float scale, const char *windowTitle);
        ~GLWindow();
        void mainLoop(Image* (*func)(bool renderOnChange, bool renderNow));
        void draw(Image *img);
        GLWindowState state;
    private:
        const char *title;
        void genTexture(int fbWidth = 0, int fbHeight = 0);
        std::string vertString, fragString;
        GLuint tex, vert, frag, prog, vao;
        void loadShader(GLuint *s, GLenum shaderType, char const *fname);
        GLWindowUI ui;
        void loadUI();
};

void mouseCallback(GLFWwindow *, double, double);
void keyCallback(GLFWwindow *, int, int, int, int);

#endif
