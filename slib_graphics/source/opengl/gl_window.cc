#ifdef WindowsBuild
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINDOWS 0x0601
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include "engine_settings.h"
#include "gl_window.h"

namespace lib_graphics {
GlWindow::GlWindow() {
  if (!glfwInit()) return;
  fullscreen_setting_ = g_settings.Fullscreen();
  windowed_setting_ = g_settings.Windowed();
  CreateRenderWindow();
}

GlWindow::~GlWindow() {
  glfwDestroyWindow(static_cast<GLFWwindow *>(load_context_));
  glfwDestroyWindow(static_cast<GLFWwindow *>(window_));
  glfwTerminate();
}

void GlWindow::CloseWindow() {
  glfwSetWindowShouldClose(static_cast<GLFWwindow *>(window_), GL_TRUE);
}

void GlWindow::SwapBuffers() {
  if (g_settings.VSync())
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);
  glfwSwapBuffers(static_cast<GLFWwindow *>(window_));
}

int GlWindow::ShouldClose() {
  if (window_) return glfwWindowShouldClose(static_cast<GLFWwindow *>(window_));
  return -1;
}

void GlWindow::SetRenderContext() {
  if (!render_claimed_ && window_) {
    glfwMakeContextCurrent(static_cast<GLFWwindow *>(window_));
    render_claimed_ = true;
  }
}

void GlWindow::SetLoadContext() {
  if (!load_claimed_ && load_context_) {
    glfwMakeContextCurrent(static_cast<GLFWwindow *>(load_context_));
    load_claimed_ = true;
  }
}

void *GlWindow::GetWindowHandle() const { return window_; }

bool GlWindow::NeedsRestart() const {
  return fullscreen_setting_ != g_settings.Fullscreen() ||
         windowed_setting_ != g_settings.Windowed();
}

void GlWindow::Rebuild() {
  SetRenderContext();
  glfwDestroyWindow(static_cast<GLFWwindow *>(load_context_));
  glfwDestroyWindow(static_cast<GLFWwindow *>(window_));
  load_context_ = nullptr;
  window_ = nullptr;
  glfwTerminate();
  glfwInit();

  fullscreen_setting_ = g_settings.Fullscreen();
  windowed_setting_ = g_settings.Windowed();
  CreateRenderWindow();
}

bool GlWindow::CheckCapabilities() {
  glewExperimental = GL_TRUE;
  cu::AssertError(glewInit() == GLEW_OK, "Failed to initialize glew.", __FILE__,
                  __LINE__);

  GLint limit;
  glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &limit);
  gpu_capabilities_.max_fragment_uniforms = limit;
  glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &limit);
  gpu_capabilities_.max_vertex_uniforms = limit;
  glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, &limit);
  gpu_capabilities_.max_geometry_uniforms = limit;

  ct::string gl_version = (const char *)glGetString(GL_VERSION);
  gpu_capabilities_.version = std::stof(gl_version);
  cu::Log("OpenGl version: " + std::to_string(gpu_capabilities_.version),
          __FILE__, __LINE__);
  return true;
}

void GlWindow::CreateRenderWindow() {
  render_claimed_ = false;
  load_claimed_ = false;
  int major_version[] = {4, 4, 4, 4, 4, 4, 4, 3};
  int minor_version[] = {6, 5, 4, 3, 2, 1, 0, 3};
  window_ = nullptr;
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  // glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
  // glfwWindowHint(GLFW_SAMPLES, 8);

  /*int monitor_count = 0;
  auto monitors = glfwGetMonitors(&monitor_count);
  for (int i = 0; i < monitor_count; ++i) {
    ct::string monitor_name = glfwGetMonitorName(monitors[i]);

    int video_mode_count = 0;
    auto video_modes = glfwGetVideoModes(monitors[i], &video_mode_count);
    for (int ii = 0; ii < video_mode_count; ++ii) {
    }
  }*/

  if (fullscreen_setting_) {
    auto primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary_monitor);
    current_dim_ = {mode->width, mode->height};

    if (windowed_setting_) {
      glfwWindowHint(GLFW_RED_BITS, mode->redBits);
      glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
      glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
      glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

      for (int i = 0; i < 8; ++i) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_version[i]);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_version[i]);

        window_ = glfwCreateWindow(current_dim_.first, current_dim_.second,
                                   "Fract Editor", primary_monitor, NULL);
        if (window_) break;
      }
    } else {
      for (int i = 0; i < 8; ++i) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_version[i]);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_version[i]);

        window_ = glfwCreateWindow(current_dim_.first, current_dim_.second,
                                   "Fract Editor", primary_monitor, NULL);
        if (window_) break;
      }
    }
  } else {
    current_dim_ = {g_settings.WindowedWidth(), g_settings.WindowedHeight()};
    for (int i = 0; i < 8; ++i) {
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_version[i]);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_version[i]);

      window_ = glfwCreateWindow(current_dim_.first, current_dim_.second,
                                 "Fract Editor", NULL, NULL);
      if (window_) break;
    }
  }

  if (window_) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    load_context_ = glfwCreateWindow(640, 480, "load window", NULL,
                                     static_cast<GLFWwindow *>(window_));
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  }

  cu::AssertError(window_ != nullptr && load_context_ != nullptr,
                  "Window creation failed, graphics card must support "
                  "at least OpenGL 3.3.",
                  __FILE__, __LINE__);
}
}  // namespace lib_graphics
