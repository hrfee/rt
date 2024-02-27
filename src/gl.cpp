#include "gl.hpp"

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cmath>

#include "tga.hpp"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define MOVE_SPEED 0.1f

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
    state.reloadMap = false;
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
    state.rc.globalShininess = -1.f;
    // Indicate unset, so map can set it for us the first time round.
    state.rc.baseBrightness = -1.f;
    state.rc.refractiveIndex = 1.52f;
    state.rc.maxBounce = 100;
    state.rc.triangles = true;
    state.rc.spheres = true;
    state.rc.planeOptimisation = true;
    state.useOptimizedMap = false;
    state.rc.showDebugObjects = false;
    state.kdLevel = 1;
    state.renderOptimizedHierarchy = false;

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
                state.rc.renderNow = ImGui::Checkbox("Phong specular", &(state.rc.specular)) ? true : state.rc.renderNow;
                if (state.rc.specular) {
                    state.rc.renderNow = ImGui::SliderFloat("Global shininess (n)", &(state.rc.globalShininess), 0.f, 100.f) ? true : state.rc.renderNow;
                }
                break;
        };
        state.rc.renderNow = ImGui::Checkbox("Render spheres", &(state.rc.spheres)) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::Checkbox("Render triangles", &(state.rc.triangles)) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::Checkbox("Use container quad optimisation", &(state.rc.planeOptimisation)) ? true : state.rc.renderNow;
        
        state.rc.renderNow = ImGui::Checkbox("Use KD optimised map", &(state.useOptimizedMap)) ? true : state.rc.renderNow;
        if (state.useOptimizedMap) {
            state.rc.renderNow = ImGui::SliderInt("KD divide level", &(state.kdLevel), 1, 100);
            state.rc.renderNow = ImGui::Checkbox("Draw cube around sections", &(state.rc.showDebugObjects)) ? true : state.rc.renderNow;
            ImGui::Checkbox("Show hierarchy", &(state.renderOptimizedHierarchy));
            if (state.renderOptimizedHierarchy) {
                renderTree(state.optimizedMap);
            }
        } else {
            state.renderOptimizedHierarchy = false;
        }

        state.rc.renderNow = ImGui::InputInt("Max ray bounces", &(state.rc.maxBounce), 1, 10) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::SliderFloat("Refractive Index", &(state.rc.refractiveIndex), 0.f, 2.f) ? true : state.rc.renderNow;
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
        ImGui::InputText(".tga path", &(state.filePath));
        ui.saveToTGA = ImGui::Button("Save", ImVec2(90, 25));
        ImGui::Text("Map Loading");
        ImGui::InputText(".map path", &(state.mapPath));
        state.reloadMap = ImGui::Button("Reload Map (50/50 SEGV)", ImVec2(90, 25));
        if (state.reloadMap) {
            state.rc.baseBrightness = -1.f;
            state.rc.globalShininess = -1.f;
        }
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
        state.rc.renderNow = ImGui::InputFloat3("Position", (float*)&(state.rc.manualPosition)) ? true : state.rc.renderNow;
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
<M>: Movement mode (use mouse+WASD to control camera)
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
    if (!(w->state.mouse.enabled)) return;
    if (key == GLFW_KEY_W) {
        w->state.mouse.moveForward = (action == GLFW_PRESS) ? MOVE_SPEED : 0.f;
    } else if (key == GLFW_KEY_S) {
        w->state.mouse.moveForward = (action == GLFW_PRESS) ? -1.f*MOVE_SPEED : 0.f;
    } else if (key == GLFW_KEY_D) {
        w->state.mouse.moveSideways = (action == GLFW_PRESS) ? 1.f*MOVE_SPEED : 0.f;
    } else if (key == GLFW_KEY_A) {
        w->state.mouse.moveSideways = (action == GLFW_PRESS) ? -1.f*MOVE_SPEED : 0.f;
    }
}

GLWindow::~GLWindow() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GLWindow::renderTree(Container *c, int tabIndex) {
    ImVec4 containerColor(1.f, 1.f, 1.f, 1.f);
    if (tabIndex == 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::Text("root(%d, (%.3f, %.3f %.3f), (%.3f, %.3f, %.3f));\n",
                c->size,
                c->a.x, c->a.y, c->a.z,
                c->b.x, c->b.y, c->b.z
        );
        ImGui::PopStyleColor();
    } else {
        auto prefix = std::string(tabIndex*2, ' ');
        Bound *bo = c->start;

        while (bo != c->end->next) {
            Shape *current = bo->s;
            if (current->t != NULL && current->debug) {
                containerColor = ImVec4(current->color.x, current->color.y, current->color.z, 1.f);
                break;
            }
            bo = bo->next;
        }
        char splitAxis = 'x';
        if (c->splitAxis == 1) splitAxis = 'y';
        else if (c->splitAxis == 2) splitAxis = 'z';
        ImGui::PushStyleColor(ImGuiCol_Text, containerColor);
        ImGui::Text("%s node(%d%c, (%.3f, %.3f %.3f), (%.3f, %.3f, %.3f));\n",
                prefix.c_str(), c->size, splitAxis,
                c->a.x, c->a.y, c->a.z,
                c->b.x, c->b.y, c->b.z
        );
        ImGui::PopStyleColor();
    }
    Bound *bo = c->start;

    int children = 0;
    while (bo != c->end->next) {
        Shape *current = bo->s;
        if (current->c != NULL) {
            renderTree(current->c, tabIndex+1);
        } else if (!(current->debug)) {
            children++;
        }
        bo = bo->next;
    }
    auto prefix2 = std::string((tabIndex+1)*2, ' ');
    if (children != 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, containerColor);
        ImGui::Text("%s%d leaves;\n",
                prefix2.c_str(), children
        );
        ImGui::PopStyleColor();
    }
}

