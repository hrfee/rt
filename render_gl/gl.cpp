#include "gl.hpp"

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <math.h>

void GLWindow::loadShader(GLuint *s, GLenum shaderType, char const *fname) {
    std::ifstream f(fname);
    if (!f) {
        throw std::runtime_error("Failed to read shader: " + std::string(fname));
    }

    std::stringstream buf;
    buf << f.rdbuf();
   
    std::string shaderString = buf.str();
    const char *cString = shaderString.c_str();
    
    *s = glCreateShader(shaderType);
    glShaderSource(*s, 1, &cString, NULL);
    glCompileShader(*s);
    
    // Wait for compilation to finish, and check if it succeeded.
    GLint status = 0;
    glGetShaderiv(*s, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        throw std::runtime_error(std::string(fname) + " compilation failed");
    }
}

GLWindow::GLWindow(int width, int height, float scale, const char *windowTitle) {
    state.fbWidth = width;
    state.fbHeight = height;
    state.w = float(width)*scale;
    state.h = float(height)*scale;
    state.scale = scale;
    title = windowTitle;
    int success = glfwInit();
    if (!success) {
        char const* error = nullptr;
        int code = glfwGetError(&error);
        glfwTerminate();
        throw std::runtime_error("Failed init (" + std::to_string(code) + "): " + std::string(error));
    }

    glfwInitHint(0x00053001, 0x00038002);
    // The line above is equivalent to below, but won't fail to compile on older versions.
    // glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_PREFER_LIBDECOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    window = glfwCreateWindow(state.fbWidth, state.fbHeight, title, nullptr, nullptr);
    if (!window) {
        char const* error = nullptr;
        int code = glfwGetError(&error);
        glfwTerminate();
        throw std::runtime_error("Failed creating window (" + std::to_string(code) + "): " + std::string(error));
    }
  
    glfwSetWindowUserPointer(window, &state);

    glfwMakeContextCurrent(window);
    
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed loading OpenGL API");
    }

	glEnable( GL_FRAMEBUFFER_SRGB );

    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);

    // Create shader
    prog = glCreateProgram();
    loadShader(&vert, GL_VERTEX_SHADER, "render_gl/pixel.vert");
    loadShader(&frag, GL_FRAGMENT_SHADER, "render_gl/pixel.frag");
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    // Wait for shader program to link, and check if it succeeded.
    GLint status = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        throw std::runtime_error("Shader linking failed");
    }

    glfwGetFramebufferSize(window, &(state.fbWidth), &(state.fbHeight));
    state.w = float(state.fbWidth) * scale;
    state.h = float(state.fbHeight) * scale;
    glViewport(0, 0, state.fbWidth, state.fbHeight);
    
    // Create texture
    genTexture();

    // Bind controls
    {
        // Mouse
        state.mouse.enabled = false;
        state.mouse.sensitivity = 0.003f;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursorPosCallback(window, &mouseCallback);
    }
    {
        // Keyboard
        glfwSetKeyCallback(window, &keyCallback);
    }
}

void GLWindow::genTexture(int fbWidth, int fbHeight) {
    if (fbWidth == 0) fbWidth = state.fbWidth;
    if (fbHeight == 0) fbHeight = state.fbWidth;
    int w = int(float(fbWidth) * state.scale);
    int h = int(float(fbHeight) * state.scale);
    std::fprintf(stderr, "Resizing to %dx%d\n", w, h);
    GLuint newTex = 0;
    glGenTextures(1, &newTex);
    glBindTexture(GL_TEXTURE_2D, newTex);
    glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            GL_SRGB8_ALPHA8,
            GLsizei(w), GLsizei(h)
    );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

    if (newTex != 0) {
        // No-op if no texture is bound already, so no checks needed.
        glDeleteTextures(1, &tex);
        tex = newTex;
    }
}

void GLWindow::draw(Image *img) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0, 0,
            GLsizei(state.w), GLsizei(state.h),
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,
            img->rgbxImg
    );

    glUseProgram(prog);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
    glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLWindow::mainLoop(Image* (*func)()) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    
        glfwGetFramebufferSize(window, &(state.fbWidth), &(state.fbHeight));
        if (state.fbWidth != state.prevFbWidth || state.fbHeight != state.prevFbHeight) {
            glViewport(0, 0, state.fbWidth, state.fbHeight);
            genTexture(state.fbWidth, state.fbHeight);
            state.prevFbWidth = state.fbWidth;
            state.prevFbHeight = state.fbHeight;
            state.w = float(state.fbWidth) * state.scale;
            state.h = float(state.fbHeight) * state.scale;
        }

        Image *img = func();
        draw(img);

        glfwSwapBuffers(window);
    }
}

void mouseCallback(GLFWwindow *window, double x, double y) {
    GLWindowState* state = static_cast<GLWindowState*>(glfwGetWindowUserPointer(window));
    if (state->mouse.enabled) {;
        float dx = x - state->mouse.prevX;
        float dy = y - state->mouse.prevY;
        state->mouse.phi -= dx * state->mouse.sensitivity;
        state->mouse.theta -= dy * state->mouse.sensitivity;
        if (state->mouse.theta > M_PI/2.f)
            state->mouse.theta = M_PI/2.f;
        else if (state->mouse.theta < -M_PI/2.f)
            state->mouse.theta = -M_PI/2.f;
    }
    state->mouse.prevX = x;
    state->mouse.prevY = y;
}

void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int mod) {
    GLWindowState* state = static_cast<GLWindowState*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_M && action == GLFW_PRESS && !state->mouse.enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        state->mouse.enabled = true;
    } else if ((key == GLFW_KEY_M || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS && state->mouse.enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        state->mouse.enabled = false;
    }
}

GLWindow::~GLWindow() {
    glfwTerminate();
}
