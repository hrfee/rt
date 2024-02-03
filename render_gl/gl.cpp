#include "gl.hpp"

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cmath>

#include "../render_tga/tga.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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
    state.requestedFbW = width;
    state.requestedFbH = height;
    state.w = float(width)*scale;
    state.h = float(height)*scale;
    state.scale = scale;
    state.prevScale = scale;
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
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
 
    glfwSetWindowUserPointer(window, this);

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

    // Apply the actual window size and generate the texture for it,
    // and bind the callback so this happens on resizes.
    glfwGetFramebufferSize(window, &(state.fbWidth), &(state.fbHeight));
    resizeWindowCallback(window, state.fbWidth, state.fbHeight);
    glfwSetFramebufferSizeCallback(window, &resizeWindowCallback);
    
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

    // Load UI
    loadUI();
}

void GLWindow::loadUI() {
    ui.ctx = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    state.rc.renderOnChange = false;
    ui.renderMode = 1;
    state.rc.distanceDivisor = 1.f;
    state.rc.baseBrightness = 0.f;
    state.rc.triangles = true;
    state.rc.spheres = true;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

void GLWindow::reloadTexture() {
    std::fprintf(stderr, "Will render @ %dx%d\n", state.w, state.h);
    GLuint newTex = 0;
    glGenTextures(1, &newTex);
    glBindTexture(GL_TEXTURE_2D, newTex);
    glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            GL_SRGB8_ALPHA8,
            GLsizei(state.w), GLsizei(state.h)
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

std::string GLWindow::frameInfo() {
    std::ostringstream out;
    out.precision(5);
    out << "Frame took " << int(state.lastRenderTime*1000.f) << "ms ";
    out << "(";
    if (state.lastRenderTime > 1.f) {
        out << state.lastRenderTime << "s, ";
        out.precision(2);
        out << 1.f/state.lastRenderTime;
    } else {
        out << int(1.f/state.lastRenderTime);
    }
    out << "fps) ";
    out << "at " << state.lastRenderW << 'x' << state.lastRenderH << ".";
    return out.str();
}

namespace {
    const char *modes[] = {"Raycasting", "Raycasting w/ Reflections", "Basic Lighting"};
}

void GLWindow::showUI() {
    ImGui::Begin("render controls");
    {
        state.rc.renderNow = ImGui::Combo("Render Mode", &(ui.renderMode), modes, IM_ARRAYSIZE(modes));
        switch(ui.renderMode) {
            case 0:
                state.rc.reflections = false;
                state.rc.lighting = false;
                break;
            case 1:
                state.rc.reflections = true;
                state.rc.lighting = false;
                break;
            case 2:
                state.rc.reflections = true;
                state.rc.lighting = true;
                state.rc.renderNow = ImGui::SliderFloat("Inverse square distance divisor", &(state.rc.distanceDivisor), 1.f, 100.f) ? true : state.rc.renderNow;
                state.rc.renderNow = ImGui::SliderFloat("Base brightness", &(state.rc.baseBrightness), 0.f, 1.f) ? true : state.rc.renderNow;
                break;
        };
        state.rc.renderNow = ImGui::Checkbox("Render spheres", &(state.rc.spheres)) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::Checkbox("Render triangles", &(state.rc.triangles)) ? true : state.rc.renderNow;
    }
    ImGui::End();
    ImGui::Begin("output controls");
    {
        ImGui::Checkbox("Render on parameter change (may cause unresponsiveness, scale limit will be reduced)", &(state.rc.renderOnChange));

        // ImGui::BeginChild("Window Resolution");
        ImGui::Text("Window Resolution");
        if (state.rc.renderOnChange) ImGui::Text("Press <Enter> to apply changes.");
        else ImGui::Text("Changes will be applied when \"Render\" is pressed.");
        ui.widthApply = ImGui::InputInt("Width", &(state.requestedFbW), 1, 100, state.rc.renderOnChange ? ImGuiInputTextFlags_EnterReturnsTrue : ImGuiInputTextFlags_None);
        ui.heightApply = ImGui::InputInt("Height", &(state.requestedFbH), 1, 100, state.rc.renderOnChange ? ImGuiInputTextFlags_EnterReturnsTrue : ImGuiInputTextFlags_None);
        // ImGui::EndChild();

        ImGui::SliderFloat("Resolution Scale", &(state.scale), 0.f, state.rc.renderOnChange ? 2.f : 10.f);
        ImGui::Text("Effective resolution: %dx%d", int(float(state.requestedFbW) * state.scale), int(float(state.requestedFbH) * state.scale));
        if (!state.rc.renderOnChange) state.rc.renderNow = ImGui::Button("Render", ImVec2(120, 40));
        if (state.lastRenderTime != 0.f)
            ImGui::Text(frameInfo().c_str());
        
        ImGui::Text("Dump render to .tga file");
        ImGui::InputText("Filepath", &(state.filePath));
        ui.saveToTGA = ImGui::Button("Save", ImVec2(90, 25));
    }
    ImGui::End();
    ImGui::Begin("camera controls");
    {
        if (state.mouse.enabled) {
            ImGui::Text("--movement enabled - press <M> or <ESC> to escape--");
        }
        // ImGui::BeginChild("Camera Angle");
        ImGui::Text("Camera Angle");
        ImGui::SliderFloat("phi (left/right) (radians)", &(state.mouse.phi), -M_PI, M_PI);
        ImGui::SliderFloat("theta (up/down) (radians)", &(state.mouse.theta), -M_PI/2.f, M_PI/2.f);
        // ImGui::EndChild();
        ImGui::SliderFloat("Field of View (degrees)", &(state.fovDeg), 0.f, 180.f);
    }
    ImGui::End();
    showKeyboardHelp();
}


void GLWindow::mainLoop(Image* (*func)(RenderConfig *c)) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        state.rc.renderNow = false;
        ui.widthApply = false, ui.heightApply = false;
        ui.saveToTGA = false;

        showUI();

        if ((state.rc.renderNow || (state.rc.renderOnChange && (ui.widthApply || ui.heightApply))) && (state.requestedFbW != state.fbWidth || state.requestedFbH != state.fbHeight)) {
            glfwSetWindowSize(window, state.requestedFbW, state.requestedFbH);
        }

        if ((state.rc.renderNow || state.rc.renderOnChange) && state.scale != state.prevScale) {
            reloadScaledResolution();
        }
        
        Image *img = func(&(state.rc));
        draw(img);
      
        if (ui.saveToTGA && !state.filePath.empty()) {
            writeTGA(img, state.filePath, frameInfo());
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void resizeWindowCallback(GLFWwindow *window, int width, int height) {
    GLWindow* w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    w->state.fbWidth = width;
    w->state.fbHeight = height;
    w->state.requestedFbW = width;
    w->state.requestedFbH = height;
    glViewport(0, 0, width, height);
    w->reloadScaledResolution();
}

void GLWindow::reloadScaledResolution() {
    state.w = float(state.fbWidth) * state.scale;
    state.h = float(state.fbHeight) * state.scale;
    reloadTexture();
}

void mouseCallback(GLFWwindow *window, double x, double y) {
    GLWindow* w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    if (w->state.mouse.enabled) {;
        float dx = x - w->state.mouse.prevX;
        float dy = y - w->state.mouse.prevY;
        w->state.mouse.phi -= dx * w->state.mouse.sensitivity;
        w->state.mouse.theta -= dy * w->state.mouse.sensitivity;
        if (w->state.mouse.theta > M_PI/2.f)
            w->state.mouse.theta = M_PI/2.f;
        else if (w->state.mouse.theta < -M_PI/2.f)
            w->state.mouse.theta = -M_PI/2.f;
        // Functionally this makes no difference, but it makes the values look nicer on the UI slider.
        if (w->state.mouse.phi > M_PI) w->state.mouse.phi = -M_PI + (w->state.mouse.phi - M_PI);
        if (w->state.mouse.phi < -M_PI) w->state.mouse.phi = M_PI - (w->state.mouse.phi + M_PI);
    }
    w->state.mouse.prevX = x;
    w->state.mouse.prevY = y;
}


void showKeyboardHelp() {
    ImGui::Begin("keyboard controls");
    {
        ImGui::Text(R"(
<M>: Movement mode (use mouse to control camera)
   - Frames will be rendered with every movement,
     so make sure resolution scale is low enough.
<ESC>: Exit
<Ctrl+LMB> on a slider: set exact value
        )");
    }
    ImGui::End();
}

void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int mod) {
    // Ignore if any imgui fields are being typed in.
    if (ImGui::GetIO().WantTextInput) return;
    GLWindow* w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_M && action == GLFW_PRESS && !w->state.mouse.enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        w->state.mouse.enabled = true;
    } else if ((key == GLFW_KEY_M || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS && w->state.mouse.enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        w->state.mouse.enabled = false;
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && !w->state.mouse.enabled) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

GLWindow::~GLWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
