#ifndef RENDER_GL
#define RENDER_GL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../img/img.hpp"
#include "../map/map.hpp"
#include <string>

class GLWindow {
    public:
        GLFWwindow* window;
        GLWindow(int w, int h);
        ~GLWindow();
        void mainLoop(Image* (*func)());
        void draw(Image *img);

    private:
        std::string vertString, fragString;
        GLuint tex, vert, frag, prog, vao;
        int w, h;
        void loadShader(GLuint *s, GLenum shaderType, char const *fname);
};

#endif
