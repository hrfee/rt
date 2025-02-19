#include "gl.hpp"

#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cmath>

#include "accel.hpp"
#include "glad/glad.h"
#include "imgui.h"
#include "shape.hpp"
#include "tga.hpp"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "aa.hpp"

#define MOVE_SPEED 1.f

#define FONT_SIZE 18.f
#define FONT_SIZE_HIDPI 15.f

#define ICON_SAVE ""
#define CHAR_SAVE u''
#define ICON_REFRESH ""
#define CHAR_REFRESH u''
#define ICON_BIN ""
#define CHAR_BIN u''

// Another note: below might not be true, each pair in this array seems to be a range, so doing the same twice sorts it.
// Note: Make sure these are in the right order when adding more!
// an easy way to find it is to load the font in gnome-font-viewer,
// the order of the icons should be matched here.
namespace {
    static const ImWchar16 icon_ranges[] = { CHAR_BIN, CHAR_BIN, CHAR_REFRESH, CHAR_REFRESH, CHAR_SAVE, CHAR_SAVE };
};

void SubImage::regen() {
    GLuint nTex = 0; 
    glGenTextures(1, &nTex);
    glBindTexture(GL_TEXTURE_2D, nTex);
    glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            GL_SRGB8_ALPHA8,
            GLsizei(img->w), GLsizei(img->h)
    );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

    if (nTex != 0) {
        // No-op if no texture is bound already, so no checks needed.
        glDeleteTextures(1, &tex);
        tex = nTex;
        sizeChanged = false;
    }
}

void SubImage::upload() {
    if (!dirty && !sizeChanged) return;
    if (sizeChanged) {
        regen();
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0, 0,
            GLsizei(img->w), GLsizei(img->h),
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,
            img->rgbxImg
    );
    dirty = false;
}

void SubImage::show() {
    ImGui::Image((void*)(intptr_t)tex, ImVec2(img->w, img->h));
}

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
    state.currentlyLoading = false;
    state.lastRenderTime = 0.f;
    state.camPresetNames = NULL;
    state.camPresetCount = 0;
    state.camPresetIndex = 0;
    state.dumpToCsv = false;
    state.csvDirty = false;
    state.objectCount = 0;
    state.plCount = 0;
    state.objectIndex = -1;
    state.materialIndex = -1;
    title = windowTitle;
    ui.hide = false;
    int success = glfwInit();
    if (!success) {
        char const* error = nullptr;
        int code = glfwGetError(&error);
        glfwTerminate();
        throw std::runtime_error("Failed init (" + std::to_string(code) + "): " + std::string(error));
    }

    // glfwInitHint(0x00053001, 0x00038002);
    // The line above is equivalent to below, but won't fail to compile on older versions.
    // glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_PREFER_LIBDECOR);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    //
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
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
        state.mouse.speedMultiplier = 1.f;
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

    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    std::printf("Got window scale %f, %f\n", xscale, yscale);
    int monitorCount;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);
    if (monitorCount > 0) {
        float mxscale, myscale;
        glfwGetMonitorContentScale(monitors[0], &mxscale, &myscale);
        std::printf("Got monitor scale %f, %f, rounded to %d\n", xscale, yscale, int(xscale));
        if (mxscale > xscale || myscale > yscale) {
            xscale = mxscale;
            yscale = myscale;
        }
    }

    // xscale = 1.f;

    ui.ctx = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.DisplayFramebufferScale = ImVec2(xscale, yscale);
    ImFontConfig cfg;
    float size = FONT_SIZE;
    if (xscale > 1.5f) size = FONT_SIZE_HIDPI;
    cfg.SizePixels = size * xscale;
    io.Fonts->AddFontFromFileTTF("third_party/fonts/static/HankenGrotesk-SemiBold.ttf", cfg.SizePixels, &cfg);
    cfg.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("third_party/fonts/icon/remixicon.ttf", cfg.SizePixels, &cfg, icon_ranges);
    io.FontGlobalScale = 1.f / xscale;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.Fonts->AddFontFromFileTTF("third_party/fonts/HankenGrotesk-VariableFont_wght.ttf", size);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.f;
    style.FrameRounding = 2.0f;
    // style.GrabRounding = 10.0f;
    // style.PopupRounding = 10.0f;
    // style.ScrollbarRounding = 10.0f;
    // style.TabRounding = 5.0f;
    // style.ChildRounding = 10.0f;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.FramePadding = ImVec2(6.f, 4.5f);

    // style.ScaleAllSizes(xscale);

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
    state.rc.aabs = true;

    state.rc.mtTriangleCollision = true;

    state.rc.normalMapping = false;
    state.rc.reflectanceMapping = false;

    state.rc.samplesPerPx = 1;
    state.rc.sampleMode = SamplingMode::Grid;

    state.useOptimizedMap = false;
    state.rc.showDebugObjects = false;
    state.accelDepth = 32;
    state.accelIndex = 1; // SAH
    state.accelParam = 2;
    state.accelFloatParam = 1.5f; // For now, SAH tri/sphere cost ratio
    state.useBVH = true;
    state.renderOptimizedHierarchy = false;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

void GLWindow::reloadRenderTexture() {
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

    // Unbind
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

void GLWindow::threadInfo() {
    std::ostringstream out;
    if (state.rc.threadStates == NULL) return;
    int done = 0;
    for (int i = 0; i < state.rc.nthreads; i++) {
        if (state.rc.threadStates[i] == 0) done++;
    }
    out << done << "/" << state.rc.nthreads << " threads complete.";

    ImGui::ProgressBar(float(done)/float(state.rc.nthreads), ImVec2(0.f, 0.f), out.str().c_str());
    // ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    // ImGui::Text(out.str().c_str());
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
    out << "at " << state.lastRenderW << 'x' << state.lastRenderH << " on " << state.rc.nthreads << " threads";
    if (state.rc.samplesPerPx > 1) {
        out << " with AA @ " << state.rc.samplesPerPx << "s^2/px (" << samplingModes[state.rc.sampleMode] << ")";
    }
    out << ".";
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

// FIXME: Add reporting of file load errors from texturestore/texture (potentially with throws?)
std::string GLWindow::mapLoadInfo() {
    std::ostringstream out;
    out.precision(5);
    out << "Loading \"";
    out << state.mapStats->name << "\" ";
    if (state.currentlyLoading) out << "has taken ";
    else out << "took ";
    out << int(state.lastLoadTime*1000.f) << "ms: ";
    out << state.mapStats->spheres << "s/";
    out << state.mapStats->tris << "t/";
    out << state.mapStats->lights << "l/";
    out << state.mapStats->planes << "p/";
    out << state.mapStats->aabs << "b/";
    out << state.mapStats->tex << "tex/";
    out << state.mapStats->norm << "ntex/";
    out << state.mapStats->allocs << "allocs.";
    out << " Failures: ";
    out << state.mapStats->missingObj << "o/";
    out << state.mapStats->missingTex << "tex/";
    out << state.mapStats->missingNorm << "ntex.";

    return out.str();
}

bool GLWindow::shouldntBeDoingAnything() {
    return state.currentlyRendering || state.currentlyOptimizing || state.currentlyLoading;
}

void GLWindow::disable() {
    if (shouldntBeDoingAnything()) ImGui::BeginDisabled();
}

void GLWindow::enable() {
    if (shouldntBeDoingAnything()) ImGui::EndDisabled();
}

bool GLWindow::vl(bool t) {
    state.rc.change = t ? true : state.rc.change;
    return t;
}

#define TAB
#ifdef TAB
#define IMGUIBEGIN(n) ImGui::BeginTabItem(n)
#define IMGUIEND() ImGui::EndTabItem()
#define STARTTAB(n) ImGui::BeginTabBar(n)
#define ENDTAB() ImGui::EndTabBar()
#else
#define IMGUIBEGIN(n) ImGui::Begin(n)
#define IMGUIEND() ImGui::End()
#define STARTTAB(n)
#define ENDTAB()
#endif

void GLWindow::addUI() {
    state.rc.renderNow = false;
    bool backgroundText = ui.hide;
    if (backgroundText) {
        ImGui::Begin("backgroundtext", NULL, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoInputs);
        {
            if (state.mouse.enabled) {
                ImGui::Text("--movement enabled - press <M> or <ESC> to escape--");
            }
            showKeyboardHelp();
            // ImGui::Text(std::to_string(state.mouse.speedMultiplier).c_str());
        }
        ImGui::End();
        return;
    }
    #ifdef TAB
    ImGui::Begin("controls");
    #endif
    // if (IMGUIBEGIN("output controls")) {
    ImGui::Text("--output controls--");
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
        ImGui::Columns(2, "AA");
        ImGui::SetColumnWidth(0, 500.f);
        vl(ImGui::SliderInt("Samples^2 per px", &(state.rc.samplesPerPx), 1, 16));
        vl(ImGui::Combo("AA Sampling mode", &(state.rc.sampleMode), samplingModes, IM_ARRAYSIZE(samplingModes)));
        if (!state.rc.renderOnChange) state.rc.renderNow = ImGui::Button(
                (state.useOptimizedMap && state.staleAccelConfig) ? "Optimize" : "Render",
                ImVec2(120, 40)
        );
        ImGui::NextColumn();
        ImGui::Text("Sample Grid");
        images.at(0)->show();
        ImGui::Columns();
        enable();
        if (state.currentlyRendering) {
            if (ImGui::Button("Cancel", ImVec2(120, 40))) {
                for (int i = 0; i < state.rc.nthreads; i++) {
                    state.rc.threadStates[i] = -1;
                }
            }
            threadInfo();
        }
        if (state.lastRenderTime != 0.f) {
            ImGui::Text(frameInfo().c_str());
        }
       
        ImGui::Checkbox("Dump future render stats to CSV buffer", &(state.dumpToCsv));

        ImGui::Text("Dump render to .tga file");
        ui.saveToTGA = ImGui::Button(ICON_SAVE);
        ImGui::SameLine();
        ImGui::InputText(".tga path", &(state.filePath));
      
        ImGui::Text("Map Loading");
        disable();
        bool reloadMap = ImGui::Button(ICON_REFRESH);
        ImGui::SameLine();
        enable();
        ImGui::InputText(".map path", &(state.mapPath));

        disable();
        if (reloadMap) {
            state.reloadMap = true;
        }
        if (state.lastLoadTime != 0.f) {
            ImGui::Text(mapLoadInfo().c_str());
        }
        enable();
        // IMGUIEND();
    }
    STARTTAB("Tabs");
    if (IMGUIBEGIN("render controls")) {
        disable();
        vl(ImGui::Combo("Render Mode", &(ui.renderMode), modes, IM_ARRAYSIZE(modes)));
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
                vl(ImGui::SliderFloat("Inverse square distance divisor", &(state.rc.distanceDivisor), 0.1f, 5.f));
                vl(ImGui::SliderFloat("Base brightness", &(state.rc.baseBrightness), 0.f, 1.f));
                state.rc.specular = true;
                // trig(ImGui::Checkbox("Phong specular", &(state.rc.specular)));
                if (state.rc.specular) {
                    vl(ImGui::SliderFloat("Global shininess (phong) (n)", &(state.rc.globalShininess), 0.f, 100.f));
                }
                break;
        };
        vl(ImGui::Checkbox("Render spheres", &(state.rc.spheres)));
        vl(ImGui::Checkbox("Render triangles", &(state.rc.triangles)));
        vl(ImGui::Checkbox("Render AABs (boxes)", &(state.rc.aabs)));
        /* This option is no longer exposed, MT is used by default except for planes.
         * Can be modified with #define PREFER_MT in shape.cpp.
        vl(ImGui::Checkbox("Möller-Trumbore tri collision (faster, allows textures)", &(state.rc.mtTriangleCollision)));
        */
        vl(ImGui::Checkbox("Normal mapping", &(state.rc.normalMapping)));
        vl(ImGui::Checkbox("Reflectance mapping", &(state.rc.reflectanceMapping)));

        vl(ImGui::SliderInt("Rendering Threads", &(state.threadCount), 1, state.maxThreadCount));
        
        vl(ImGui::Checkbox("Use/Generate KD/BVH Optimization", &(state.useOptimizedMap)));
        if (state.useOptimizedMap) {
            ImGui::Text("note: un-baked transforms on objects will cause weirdness.");
            // vl(ImGui::Checkbox("Use proper BVH", &(state.useBVH)));
            vl(ImGui::SliderInt("Max hierarchy depth", &(state.accelDepth), 1, 100));
            vl(ImGui::Checkbox("Draw cube around volumes", &(state.rc.showDebugObjects)));
            ImGui::Checkbox("Show hierarchy", &(state.renderOptimizedHierarchy));
            if (state.renderOptimizedHierarchy) {
                renderTree(state.optimizedMap);
            }
            vl(ImGui::Combo("Acceleration method", &(state.accelIndex), accelerators, IM_ARRAYSIZE(accelerators)));
            if (state.accelIndex == Accel::BiTree || state.accelIndex == Accel::FalseOctree) {
                vl(ImGui::SliderInt("Max leaves per node", &(state.accelParam), 1, 100));
            } else if (state.accelIndex == Accel::SAH) {
                vl(ImGui::SliderFloat("SAH Triangle/Sphere cost ratio", &(state.accelFloatParam), 0.1f, 100.f));
            }
            if (state.lastOptimizeTime != 0.f) {
                ImGui::Text(acceleratorInfo().c_str());
            }
        } else {
            state.renderOptimizedHierarchy = false;
            vl(ImGui::Checkbox("Show debug objects", &(state.rc.showDebugObjects)));

        }

        vl(ImGui::InputInt("Max ray bounces", &(state.rc.maxBounce), 1, 10));
        vl(ImGui::SliderFloat("Refractive Index", &(state.rc.refractiveIndex), 0.f, 2.f));
        enable();
        IMGUIEND();
    }
    if (IMGUIBEGIN("camera controls")) {
        disable();
        ImGui::Text("Camera Angle");
        ImGui::SliderFloat("phi (left/right) (radians)", &(state.mouse.phi), -M_PI, M_PI);
        ImGui::SliderFloat("theta (up/down) (radians)", &(state.mouse.theta), -M_PI/2.f, M_PI/2.f);
        ImGui::SliderFloat("Field of View (degrees)", &(state.fovDeg), 0.f, 180.f);
        vl(ImGui::InputFloat3("Position", (float*)&(state.rc.manualPosition)));
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
        IMGUIEND();
    }

    if (!state.currentlyLoading && !state.currentlyOptimizing) {
        if (IMGUIBEGIN("objects")) {
            ImGui::Text("note: non-material changes (i.e. position) made when using an acceleration structure are not reflected in the unoptimized version.");
            ImGui::Combo("Object", &(state.objectIndex), state.objectNames, state.objectCount);
            showShapeEditor();
            IMGUIEND();
        }
        if (IMGUIBEGIN("materials")) {
            ImGui::Text("\"Materials\" are created for all objects with unique material properties.");
            ImGui::Text("Shared materials can be defined with \"material <name> <properties>\",");
            ImGui::Text("and applied to objects by adding \"material <name>\" (+ remove other material properties).");
            ImGui::Combo("Material", &(state.materialIndex), state.materials->names, state.materials->count);
            showMaterialEditor();
            IMGUIEND();
        }
    }

    if (state.csvStats.tellp() != 0) {
        if (IMGUIBEGIN("csv stats")) {
            ImGui::Text(state.csvStats.str().c_str());
            if (ImGui::Button("Copy to clipboard")) {
                glfwSetClipboardString(window, state.csvStats.str().c_str());
                std::printf("copied %s\n", state.csvStats.str().c_str());
            }
            if (ImGui::Button("Clear")) {
                state.csvStats.str("");
            }
            IMGUIEND();
        }
    }
    ENDTAB();
    #ifdef TAB
    ImGui::End();
    #endif
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
        state.rc.change = false;
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
            reloadRenderTexture();
            state.texDim.update();
        }

        // Wipe screen, since rendered image may not cover the whole thing
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Image *img = func(&(state.rc));

        draw(img);

        if (ui.saveToTGA && !state.filePath.empty()) {
            TGA::write(img, state.filePath, frameInfo());
        }

        for (auto windowedImage : images) {
            windowedImage->upload(); 
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

bool down(int action) {
    return action == GLFW_PRESS || action == GLFW_REPEAT;
}

void keyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int mod) {
    // Ignore if any imgui fields are being typed in.
    if (ImGui::GetIO().WantTextInput) return;
    GLWindow* w = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_M && down(action) && !w->state.mouse.enabled && !w->shouldntBeDoingAnything()) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        w->state.mouse.enabled = true;
        w->hideUI();
    } else if ((key == GLFW_KEY_M || key == GLFW_KEY_ESCAPE) && down(action) && w->state.mouse.enabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        w->state.mouse.enabled = false;
        w->showUI();
    } else if (key == GLFW_KEY_ESCAPE && down(action) && !w->state.mouse.enabled) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    } else if (key == GLFW_KEY_H && down(action)) {
        w->toggleUI();
    }
    if (!(w->state.mouse.enabled)) return;

    if (key == GLFW_KEY_LEFT_SHIFT) {
        w->state.mouse.speedMultiplier = down(action) ? 2.f : 1.f;
    }
    if (mod == GLFW_MOD_SHIFT) {
        w->state.mouse.speedMultiplier = 2.f;
    }
    if (key == GLFW_KEY_LEFT_CONTROL) {
        w->state.mouse.speedMultiplier = down(action) ? .5f : 1.f;
    }
    if (mod == GLFW_MOD_CONTROL) {
        w->state.mouse.speedMultiplier = .5f;
    }
    
    if (key == GLFW_KEY_W) {
        w->state.mouse.moveForward = down(action) ? MOVE_SPEED : 0.f;
        if (action == GLFW_RELEASE) w->state.mouse.speedMultiplier = 1.f;
    } else if (key == GLFW_KEY_S) {
        w->state.mouse.moveForward = down(action) ? -1.f*MOVE_SPEED : 0.f;
        if (action == GLFW_RELEASE) w->state.mouse.speedMultiplier = 1.f;
    } else if (key == GLFW_KEY_D) {
        w->state.mouse.moveSideways = down(action) ? 1.f*MOVE_SPEED : 0.f;
        if (action == GLFW_RELEASE) w->state.mouse.speedMultiplier = 1.f;
    } else if (key == GLFW_KEY_A) {
        w->state.mouse.moveSideways = down(action) ? -1.f*MOVE_SPEED : 0.f;
        if (action == GLFW_RELEASE) w->state.mouse.speedMultiplier = 1.f;
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
                c->min.x, c->min.y, c->min.z,
                c->max.x, c->max.y, c->max.z
        );
        ImGui::PopStyleColor();
    } else {
        auto prefix = std::string(tabIndex*2, ' ');
        Bound *bo = c->start;
        while (bo != end) {
            Shape *current = bo->s;
            Triangle *t = dynamic_cast<Triangle*>(current);
            if (t != nullptr && current->debug) {
                containerColor = ImVec4(current->mat()->color.x, current->mat()->color.y, current->mat()->color.z, 1.f);
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
                c->min.x, c->min.y, c->min.z,
                c->max.x, c->max.y, c->max.z
        );
        ImGui::PopStyleColor();
    }
    Bound *bo = c->start;

    int children = 0;
    while (bo != end) {
        Shape *current = bo->s;
        Container *subContainer = dynamic_cast<Container*>(current);
        if (subContainer != nullptr) {
            renderTree(subContainer, tabIndex+1);
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

Shape *GLWindow::getShapePointer() {
    if (state.objectIndex < 0 || state.objectIndex >= state.objectCount) return NULL;
    return state.objectPtrs[state.objectIndex];
}

Material *GLWindow::getMaterialPointer() {
    if (state.materialIndex < 0 || state.materialIndex >= state.materials->count) return NULL;
    return state.materials->ptrs[state.materialIndex];
}

void GLWindow::showMaterialEditor(Material *m) {
    if (m == NULL) m = getMaterialPointer();
    if (m == NULL) return;
    vl(ImGui::ColorEdit3("Color", (float*)&(m->color)));
    vl(ImGui::SliderFloat("Opacity", &(m->opacity), 0, 1.f));
    vl(ImGui::SliderFloat("Reflectiveness", &(m->reflectiveness), 0, 1.f));
    vl(ImGui::SliderFloat("Specular Component", &(m->specular), 0, 1.f));
    vl(ImGui::SliderFloat("Shininess (n)", &(m->shininess), 0, 100.f));
    vl(ImGui::Checkbox("Disable Lighting", &(m->noLighting)));

    if (vl(ImGui::Button(ICON_BIN "##1"))) {
        m->texId = -1;
    }
    ImGui::SameLine();
    vl(ImGui::Combo("Texture", &(m->texId), state.tex->names, state.tex->texes.size()));
    
    if (vl(ImGui::Button(ICON_BIN "##2"))) {
        m->normId = -1;
    }
    ImGui::SameLine();
    vl(ImGui::Combo("Normal Map", &(m->normId), state.norms->names, state.norms->texes.size()));
    
    if (vl(ImGui::Button(ICON_BIN "##3"))) {
        m->refId = -1;
    }
    ImGui::SameLine();
    vl(ImGui::Combo("Reflection Map", &(m->refId), state.refs->names, state.refs->texes.size()));
}

void GLWindow::showShapeEditor(Shape *shape, bool hideTransforms) {
    if (state.currentlyLoading || state.currentlyOptimizing) return;
    Shape *sh = shape;
    if (shape == NULL) sh = getShapePointer();
    if (sh == NULL) return;
    // shape == NULL to check that we're looking at something from the dropdown.
    if (shape == NULL && state.objectIndex < state.plCount) {
        PointLight *pl = (PointLight*)sh;
        ImGui::Text("Selected: Light");
        ImGui::InputFloat3("Position", (float*)&(pl->center));
        vl(ImGui::ColorEdit3("Color", (float*)&(pl->color)));
        vl(ImGui::SliderFloat("Brightness", &(pl->brightness), 0, 100.f));
        vl(ImGui::ColorEdit3("Specular Color", (float*)&(pl->specularColor)));
        return;
    }
    std::string sel = sh->name();
    ImGui::Text(sel.c_str());
    Sphere *s = dynamic_cast<Sphere*>(sh);
    Triangle *t = dynamic_cast<Triangle*>(sh);
    AAB *b = dynamic_cast<AAB*>(sh);
    CSG *c = dynamic_cast<CSG*>(sh);
    Cylinder *cl = dynamic_cast<Cylinder*>(sh);
    Cone *co = dynamic_cast<Cone*>(sh);

    if (s != nullptr) {
        if (ImGui::InputFloat3("Position", (float*)&(s->oCenter)) ||
            vl(ImGui::SliderFloat("Radius", &(s->oRadius), 0, 20.f))) {
            sh->transformDirty = true;
        }
        vl(ImGui::SliderFloat("\"Thickness\"", &(s->thickness), 0, 1.f));
    } else if (t != nullptr) {
        if (ImGui::InputFloat3("Pos A", (float*)&(t->oA)) ||
            ImGui::InputFloat3("Pos B", (float*)&(t->oB)) ||
            ImGui::InputFloat3("Pos C", (float*)&(t->oC))) {
            sh->transformDirty = true;
        }
    } else if (b != nullptr) {
        if (ImGui::InputFloat3("Min Corner", (float*)&(b->oMin)) ||
            ImGui::InputFloat3("Max Corner", (float*)&(b->oMax)) ||
            vl(ImGui::SliderInt("Face for UV scaling (x, y, z)", &(b->faceForUV), 0, 2))) {
            sh->transformDirty = true;
        }
    } else if (c != nullptr) {
        vl(ImGui::Combo("CSG Operation", (int*)&(c->relation), CSG::RelationNames, CSG::RelationCount));
        if (ImGui::Begin("csg: A")) {
            showShapeEditor(c->a, true);
            ImGui::End();
        }
        if (ImGui::Begin("csg: B")) {
            showShapeEditor(c->b, true);
            ImGui::End();
        }
    } else if (cl != nullptr) {
        if (ImGui::InputFloat3("Position", (float*)&(cl->oCenter)) ||
            vl(ImGui::SliderFloat("Radius", &(cl->oRadius), 0, 20.f)) ||
            vl(ImGui::SliderFloat("Length", &(cl->oLength), 0, 20.f)) ||
            vl(ImGui::SliderInt("Axis", &(cl->axis), 0, 2))) {
            sh->transformDirty = true;
        }
        vl(ImGui::SliderFloat("\"Thickness\"", &(cl->thickness), 0, 1.f));
    } else if (co != nullptr) {
        if (ImGui::InputFloat3("Position", (float*)&(co->oCenter)) ||
            vl(ImGui::SliderFloat("Radius", &(co->oRadius), 0, 20.f)) ||
            vl(ImGui::SliderFloat("Length", &(co->oLength), 0, 20.f)) ||
            vl(ImGui::SliderInt("Axis", &(co->axis), 0, 2))) {
            sh->transformDirty = true;
        }
        vl(ImGui::SliderFloat("\"Thickness\"", &(co->thickness), 0, 1.f));
    }

    if (t != nullptr || b != nullptr) {
        if (ImGui::Button("Recalculate UVs")) {
            state.recalcUVs = true;
        }
    }

    if (!hideTransforms) {
        ImGui::Text("transforms (relative to origin)");
        if (vl(ImGui::InputFloat3("Translate", (float*)&(sh->trans().translate))) ||
            vl(ImGui::SliderFloat3("Rotate", (float*)&(sh->trans().rotate), 0.f, 2.f*M_PI)) ||
            vl(ImGui::SliderFloat("Scale", &(sh->trans().scale), 0.f, 100.f))) {
            sh->transformDirty = true;
        }
    }

    // material params
    ImGui::Text("material");
    showMaterialEditor(sh->mat());
}
