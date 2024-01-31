#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../img/img.hpp"
#include <string>

struct GLWindowState {
    int w, h;
    int fbWidth, fbHeight;
    float scale;
    double lastFrameTime;
};

class GLWindow {
    public:
        GLFWwindow* window;
        GLWindow(int w, int h, float scale);
        ~GLWindow();
        void mainLoop(Image* (*func)());
        void draw(Image *img);
        GLWindowState state;
    private:
        void genTexture(int w = 0, int h = 0);
        std::string vertString, fragString;
        GLuint tex, vert, frag, prog, vao;
        void loadShader(GLuint *s, GLenum shaderType, char const *fname);
};

#endif
