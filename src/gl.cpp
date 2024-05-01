#include "gl.hpp"

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cmath>

#include "accel.hpp"
#include "shape.hpp"
#include "tga.hpp"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

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
    state.fbDim = {width, height, width, height};
    state.scale = scale;
    state.reloadMap = false;
    state.currentlyRendering = false;
    state.currentlyOptimizing = false;
    state.lastRenderTime = 0.f;
    state.camPresetNames = NULL;
    state.camPresetCount = 0;
    state.camPresetIndex = 0;
    state.dumpToCsv = false;
    state.csvDirty = false;
    title = windowTitle;
    ui.hide = false;
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
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    window = glfwCreateWindow(state.fbDim.w, state.fbDim.h, title, nullptr, nullptr);
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
    glfwGetFramebufferSize(window, &(state.fbDim.w), &(state.fbDim.h));
    resizeWindowCallback(window, state.fbDim.w, state.fbDim.h);
    state.fbDim.update();
    // Initially, render image/viewport at full size of window.
    
    state.vpDim.w = state.fbDim.w;
    state.vpDim.h = state.fbDim.h;
    state.vpDim.update();
    state.rDim.scaleFrom(&(state.vpDim), state.scale);
    state.texDim.w = state.rDim.w;
    state.texDim.h = state.rDim.h;
    // state.texDim.update();

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

    state.rc.threadStates = NULL;
    state.rc.renderOnChange = false;
    ui.renderMode = 1;
    state.rc.distanceDivisor = 1.f;
    // Indicate unset, so map can set it for us the first time round.
    state.rc.baseBrightness = -1.f;
    state.rc.globalShininess = -1.f;
    state.threadCount = -1;
    state.maxThreadCount = -1;

    state.rc.refractiveIndex = 1.52f;
    state.rc.maxBounce = 15;
    state.rc.triangles = true;
    state.rc.spheres = true;
    state.rc.planeOptimisation = true;
    state.useOptimizedMap = false;
    state.rc.showDebugObjects = false;
    state.accelDepth = 1;
    state.accelIndex = 1; // SAH
    state.accelParam = 2;
    state.accelFloatParam = 1.5f; // For now, SAH tri/sphere cost ratio
    state.useBVH = false;
    state.renderOptimizedHierarchy = false;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

void GLWindow::reloadTexture() {
    std::fprintf(stderr, "Will render @ %dx%d\n", state.texDim.w, state.texDim.h);
    GLuint newTex = 0;
    glGenTextures(1, &newTex);
    glBindTexture(GL_TEXTURE_2D, newTex);
    glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            GL_SRGB8_ALPHA8,
            GLsizei(state.texDim.w), GLsizei(state.texDim.h)
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
  
    generateFrameVertices();
}

void GLWindow::draw(Image *img) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0, 0,
            GLsizei(state.texDim.w), GLsizei(state.texDim.h),
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,
            img->rgbxImg
    );

    glUseProgram(prog);

    glBindVertexArray(vao);
    glUniform2fv(64, 4, (GLfloat*)&glFrameVertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    glBindVertexArray(0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

std::string GLWindow::threadInfo() {
    std::ostringstream out;
    if (state.rc.threadStates == NULL) return out.str();
    int done = 0;
    for (int i = 0; i < state.rc.nthreads; i++) {
        if (state.rc.threadStates[i] == 0) done++;
    }
    out << done << "/" << state.rc.nthreads << " threads complete.";
    return out.str();
}

std::string GLWindow::frameInfo() {
    std::ostringstream out;
    out.precision(5);
    if (state.currentlyRendering) {
        out << "Time elapsed: ";
    } else {
        out << "Frame took ";
    }
    out << int(state.lastRenderTime*1000.f) << "ms ";
    out << "(";
    if (state.lastRenderTime > 1.f) {
        out << state.lastRenderTime << "s, ";
        out.precision(2);
        out << 1.f/state.lastRenderTime;
    } else {
        out << int(1.f/state.lastRenderTime);
    }
    out << "fps) ";
    out << "at " << state.lastRenderW << 'x' << state.lastRenderH << " on " << state.rc.nthreads << " threads.";
    return out.str();
}

std::string GLWindow::acceleratorInfo() {
    std::ostringstream out;
    if (state.staleAccelConfig) return out.str();
    out.precision(5);
    out << "Generation with \"" << accelerators[state.accelIndex] << "\" ";
    if (state.useBVH) {
        out << "(BVH) ";
    }
    if (state.currentlyOptimizing) {
        out << "has taken ";
    } else {
        out << "took ";
    }
    out << int(state.lastOptimizeTime*1000.f) << "ms ";
    if (state.lastOptimizeTime > 1.f) {
        out << "(" << state.lastOptimizeTime << "s) ";
    }
    out << "at max depth " << state.accelDepth << ".";
    return out.str();
}

void GLWindow::disable() {
    if (state.currentlyRendering || state.currentlyOptimizing) ImGui::BeginDisabled();
}

void GLWindow::enable() {
    if (state.currentlyRendering || state.currentlyOptimizing) ImGui::EndDisabled();
}

void GLWindow::addUI() {
    bool backgroundText = ui.hide;
    if (backgroundText) {
        ImGui::Begin("backgroundtext", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoInputs);
        {
            if (state.mouse.enabled) {
                ImGui::Text("--movement enabled - press <M> or <ESC> to escape--");
            }
            showKeyboardHelp();
        }
        ImGui::End();
        return;
    }
    if (state.csvStats.tellp() != 0) {
        ImGui::Begin("csv stats");
        {
            ImGui::Text(state.csvStats.str().c_str());
            if (ImGui::Button("Copy to clipboard")) {
                glfwSetClipboardString(window, state.csvStats.str().c_str());
                std::printf("copied %s\n", state.csvStats.str().c_str());
            }
            if (ImGui::Button("Clear")) {
                state.csvStats.str("");
            }
        }
        ImGui::End();
    }
    ImGui::Begin("render controls");
    {
        disable();
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
                state.rc.renderNow = ImGui::SliderFloat("Inverse square distance divisor", &(state.rc.distanceDivisor), 0.1f, 5.f) ? true : state.rc.renderNow;
                state.rc.renderNow = ImGui::SliderFloat("Base brightness", &(state.rc.baseBrightness), 0.f, 1.f) ? true : state.rc.renderNow;
                state.rc.specular = true;
                // state.rc.renderNow = ImGui::Checkbox("Phong specular", &(state.rc.specular)) ? true : state.rc.renderNow;
                if (state.rc.specular) {
                    state.rc.renderNow = ImGui::SliderFloat("Global shininess (phong) (n)", &(state.rc.globalShininess), 0.f, 100.f) ? true : state.rc.renderNow;
                }
                break;
        };
        state.rc.renderNow = ImGui::Checkbox("Render spheres", &(state.rc.spheres)) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::Checkbox("Render triangles", &(state.rc.triangles)) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::SliderInt("Rendering Threads", &(state.threadCount), 1, state.maxThreadCount);
        state.rc.renderNow = ImGui::Checkbox("Use container quad optimisation", &(state.rc.planeOptimisation)) ? true : state.rc.renderNow;
        
        state.rc.renderNow = ImGui::Checkbox("Use/Generate KD/BVH Optimization", &(state.useOptimizedMap)) ? true : state.rc.renderNow;
        if (state.useOptimizedMap) {
            state.rc.renderNow = ImGui::Checkbox("Use proper BVH", &(state.useBVH)) ? true : state.rc.renderNow;
            state.rc.renderNow = ImGui::SliderInt("Max hierarchy depth", &(state.accelDepth), 1, 100);
            state.rc.renderNow = ImGui::Checkbox("Draw cube around volumes", &(state.rc.showDebugObjects)) ? true : state.rc.renderNow;
            ImGui::Checkbox("Show hierarchy", &(state.renderOptimizedHierarchy));
            if (state.renderOptimizedHierarchy) {
                renderTree(state.optimizedMap);
            }
            state.rc.renderNow = ImGui::Combo("Acceleration method", &(state.accelIndex), accelerators, IM_ARRAYSIZE(accelerators));
            if (state.accelIndex == Accel::BiTree || state.accelIndex == Accel::FalseOctree) {
                state.rc.renderNow = ImGui::SliderInt("Max leaves per node", &(state.accelParam), 1, 100) ? true : state.rc.renderNow;
            } else if (state.accelIndex == Accel::SAH) {
                state.rc.renderNow = ImGui::SliderFloat("SAH Triangle/Sphere cost ratio", &(state.accelFloatParam), 0.1f, 100.f) ? true : state.rc.renderNow;
            }
            if (state.lastOptimizeTime != 0.f) {
                ImGui::Text(acceleratorInfo().c_str());
            }
        } else {
            state.renderOptimizedHierarchy = false;
        }

        state.rc.renderNow = ImGui::InputInt("Max ray bounces", &(state.rc.maxBounce), 1, 10) ? true : state.rc.renderNow;
        state.rc.renderNow = ImGui::SliderFloat("Refractive Index", &(state.rc.refractiveIndex), 0.f, 2.f) ? true : state.rc.renderNow;
        enable();
    }
    ImGui::End();
    ImGui::Begin("output controls");
    {
        ImGui::Checkbox("Render on parameter change (may cause unresponsiveness, scale limit will be reduced)", &(state.rc.renderOnChange));

        // ImGui::BeginChild("Window Resolution");
        ImGui::Text("Window Resolution");
        // if (state.rc.renderOnChange) 
        ImGui::Text("Press <Enter> to apply w/h changes.");
        disable();
        // ImGui::Text("Changes will be applied when \"Render\" is pressed.");
        ImGui::InputInt("Width", &(state.vpDim.w), 1, 100, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::InputInt("Height", &(state.vpDim.h), 1, 100, ImGuiInputTextFlags_EnterReturnsTrue);
        // ImGui::EndChild();
        if (ImGui::Button("Fit to window")) {
            state.vpDim.w = state.fbDim.w;
            state.vpDim.h = state.fbDim.h;
        }
        if (ImGui::SliderFloat("Resolution Scale", &(state.scale), 0.f, state.rc.renderOnChange ? 2.f : 10.f) || state.vpDim.dirty()) {
            state.rDim.scaleFrom(&(state.vpDim), state.scale);
            state.vpDim.update();
        }
        ImGui::Text("Effective resolution: %dx%d", state.rDim.w, state.rDim.h);
        if (!state.rc.renderOnChange) state.rc.renderNow = ImGui::Button(
                (state.useOptimizedMap && state.staleAccelConfig) ? "Optimize" : "Render",
                ImVec2(120, 40)
        );
        enable();
        if (state.currentlyRendering && ImGui::Button("Cancel", ImVec2(120, 40))) {
            for (int i = 0; i < state.rc.nthreads; i++) {
                state.rc.threadStates[i] = -1;
            }
        }
        if (state.lastRenderTime != 0.f) {
            ImGui::Text(frameInfo().c_str());
        }
        if (state.currentlyRendering) {
            ImGui::Text(threadInfo().c_str());
        }
       
        ImGui::Checkbox("Dump future render stats to CSV buffer", &(state.dumpToCsv));

        ImGui::Text("Dump render to .tga file");
        ImGui::InputText(".tga path", &(state.filePath));
        ui.saveToTGA = ImGui::Button("Save", ImVec2(90, 25));
        ImGui::Text("Map Loading");
        ImGui::InputText(".map path", &(state.mapPath));
        disable();
        state.reloadMap = ImGui::Button("Reload Map (50/50 SEGV)", ImVec2(90, 25));
        if (state.reloadMap) {
            state.rc.baseBrightness = -1.f;
            state.rc.globalShininess = -1.f;
        }
        enable();
    }
    ImGui::End();
    ImGui::Begin("camera controls");
    {
        disable();
        ImGui::Text("Camera Angle");
        ImGui::SliderFloat("phi (left/right) (radians)", &(state.mouse.phi), -M_PI, M_PI);
        ImGui::SliderFloat("theta (up/down) (radians)", &(state.mouse.theta), -M_PI/2.f, M_PI/2.f);
        ImGui::SliderFloat("Field of View (degrees)", &(state.fovDeg), 0.f, 180.f);
        state.rc.renderNow = ImGui::InputFloat3("Position", (float*)&(state.rc.manualPosition)) ? true : state.rc.renderNow;
        enable();
        if (state.camPresetNames != NULL && state.camPresetCount != 0) {
            ImGui::Combo("Camera Preset", &(state.camPresetIndex), state.camPresetNames, state.camPresetCount);
            disable();
            if (ImGui::Button("Load preset") && state.camPresets != NULL) {
                CamPreset p = state.camPresets->at(state.camPresetIndex);
                state.mouse.phi = p.phi;
                state.mouse.theta = p.theta;
                state.fovDeg = p.fov;
                state.rc.manualPosition = p.pos;
            }
            enable();
        }
        if (ImGui::Button("Copy cam preset to clipboard")) {
            CamPreset p;
            p.name = "NAME_HERE";
            p.pos = state.rc.manualPosition;
            p.phi = state.mouse.phi;
            p.theta = state.mouse.theta;
            p.fov = state.fovDeg;
            std::string camString = encodeCamPreset(&p);
            glfwSetClipboardString(window, camString.c_str());
        }
    }
    ImGui::End();
    ImGui::Begin("keyboard controls");
    {
        showKeyboardHelp();
    }
    ImGui::End();
}


void GLWindow::mainLoop(Image* (*func)(RenderConfig *c)) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        state.rc.renderNow = false;
        ui.saveToTGA = false;

        addUI();

        // bool resizeRequested = state.rDim.dirty();
        bool windowResized = state.fbDim.dirty();
        if (windowResized) {
            // FIXME: We might need this, so probably don't clean it!
            state.fbDim.update();
        }

        // Indicates the texture has been updated.
        if (state.texDim.dirty()) {
            reloadTexture();
            state.texDim.update();
        }

        // Wipe screen, since rendered image may not cover the whole thing
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
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

void GLWindow::generateFrameVertices() {
    // Generate the triangle in screen coords that will letterbox our image if required.

    Vec2 scaled = {float(state.texDim.w), float(state.texDim.h)};
    float ar[2] = {float(state.texDim.w)/float(state.texDim.h), float(state.fbDim.w)/float(state.fbDim.h)};

    float ratio = 0.f;
    if (ar[0] > ar[1]) { // Image wider
        ratio = float(state.fbDim.w) / float(state.texDim.w);
    } else {
        ratio = float(state.fbDim.h) / float(state.texDim.h);
    }
    scaled.x = scaled.x * ratio;
    scaled.y = scaled.y * ratio;

    float nW = float(scaled.x)/float(state.fbDim.w);
    float nH = float(scaled.y)/float(state.fbDim.h);

    glFrameVertices[0] = GLfloat(0.f);
    glFrameVertices[1] = GLfloat(nH);
    glFrameVertices[2] = GLfloat(0.f);
    glFrameVertices[3] = GLfloat(0.f);
    glFrameVertices[4] = GLfloat(nW);
    glFrameVertices[5] = GLfloat(nH);
    glFrameVertices[6] = GLfloat(nW);
    glFrameVertices[7] = GLfloat(0.f);

    for (int i = 0; i < 8; i++) {
        // Letterbox x and y
        if (i % 2 == 0 && nW < 1.f) glFrameVertices[i] += 0.5f*(1.f - nW);
        else if (i % 2 == 1 && nH < 1.f) glFrameVertices[i] += 0.5f*(1.f - nH);
        glFrameVertices[i] = 2.f * (glFrameVertices[i] - 0.5f);
    }
}

void resizeWindowCallback(GLFWwindow *window, int width, int height) {
    GLWindow* w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    glViewport(0, 0, width, height);
    w->state.fbDim.w = width;
    w->state.fbDim.h = height;

    w->generateFrameVertices();
    // Window resizing does not affect render/viewport resolution.
    // w->reloadScaledResolution();
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
    ImGui::Text(R"(
<H>: Show/Hide UI
<M>: Movement mode (use mouse+WASD to control camera)
- Frames will be rendered with every movement,
 so make sure resolution scale is low enough.
- UI will be hidden initially, press <H> to bring it back up.
<ESC>: Exit
<Ctrl+LMB> on a slider: set exact value
    )");
}

void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int /* mod */) {
    // Ignore if any imgui fields are being typed in.
    if (ImGui::GetIO().WantTextInput) return;
    GLWindow* w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_M && action == GLFW_PRESS && !w->state.mouse.enabled && !w->state.currentlyRendering && !w->state.currentlyOptimizing) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        w->state.mouse.enabled = true;
        w->hideUI();
    } else if ((key == GLFW_KEY_M || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS && w->state.mouse.enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        w->state.mouse.enabled = false;
        w->showUI();
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && !w->state.mouse.enabled) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    } else if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        w->toggleUI();
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
    Bound *end = c->end->next;
    int size = c->size;
    if (c->voxelSubdiv != 0) {
        size = c->voxelSubdiv * c->voxelSubdiv * c->voxelSubdiv;
        end = c->start + (c->voxelSubdiv*c->voxelSubdiv*c->voxelSubdiv) + 1;
    }


    if (tabIndex == 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
        ImGui::Text("root(%d, (%.3f, %.3f %.3f), (%.3f, %.3f, %.3f));\n",
                size,
                c->a.x, c->a.y, c->a.z,
                c->b.x, c->b.y, c->b.z
        );
        ImGui::PopStyleColor();
    } else {
        auto prefix = std::string(tabIndex*2, ' ');
        Bound *bo = c->start;
        while (bo != end) {
            Shape *current = bo->s;
            if (current->t != NULL && current->debug) {
                containerColor = ImVec4(current->color.x, current->color.y, current->color.z, 1.f);
                break;
            }
            if (c->voxelSubdiv != 0) {
                bo++;
            } else {
                bo = bo->next;
            }
        }
        char splitAxis = 'x';
        if (c->splitAxis == 1) splitAxis = 'y';
        else if (c->splitAxis == 2) splitAxis = 'z';
        ImGui::PushStyleColor(ImGuiCol_Text, containerColor);
        ImGui::Text("%s node(%d%c, (%.3f, %.3f %.3f), (%.3f, %.3f, %.3f));\n",
                prefix.c_str(), size, splitAxis,
                c->a.x, c->a.y, c->a.z,
                c->b.x, c->b.y, c->b.z
        );
        ImGui::PopStyleColor();
    }
    Bound *bo = c->start;

    int children = 0;
    while (bo != end) {
        Shape *current = bo->s;
        if (current->c != NULL) {
            renderTree(current->c, tabIndex+1);
        } else if (!(current->debug)) {
            children++;
        }
        if (c->voxelSubdiv != 0) {
            bo++;
        } else {
            bo = bo->next;
        }
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

