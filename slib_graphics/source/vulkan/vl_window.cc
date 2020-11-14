#ifdef WindowsBuild
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINDOWS 0x0601
#define NOMINMAX
#include <windows.h>
#endif

#include <array>

#include <GL/glew.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "engine_settings.h"
#include "vl_window.h"

namespace lib_graphics {
VlWindow::VlWindow() {
  if (!glfwInit()) return;
  fullscreen_setting_ = g_settings.Fullscreen();
  windowed_setting_ = g_settings.Windowed();
  CreateRenderWindow();
}

VlWindow::~VlWindow() {
  glfwDestroyWindow(static_cast<GLFWwindow *>(load_context_));
  glfwDestroyWindow(static_cast<GLFWwindow *>(window_));
  glfwTerminate();
}

void VlWindow::CloseWindow() {
  glfwSetWindowShouldClose(static_cast<GLFWwindow *>(window_), GL_TRUE);
}

void VlWindow::SwapBuffers() {
  if (g_settings.VSync())
    glfwSwapInterval(1);
  else
    glfwSwapInterval(0);
  glfwSwapBuffers(static_cast<GLFWwindow *>(window_));
}

int VlWindow::ShouldClose() {
  if (window_) return glfwWindowShouldClose(static_cast<GLFWwindow *>(window_));
  return -1;
}

void VlWindow::SetRenderContext() {
  if (!render_claimed_ && window_) {
    glfwMakeContextCurrent(static_cast<GLFWwindow *>(window_));
    render_claimed_ = true;
  }
}

void VlWindow::SetLoadContext() {
  if (!load_claimed_ && load_context_) {
    glfwMakeContextCurrent(static_cast<GLFWwindow *>(load_context_));
    load_claimed_ = true;
  }
}

void *VlWindow::GetWindowHandle() const { return window_; }

bool VlWindow::NeedsRestart() const {
  return fullscreen_setting_ != g_settings.Fullscreen() ||
         windowed_setting_ != g_settings.Windowed();
}

void VlWindow::Rebuild() {
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

bool VlWindow::CheckCapabilities() { return glfwVulkanSupported(); }

void VlWindow::CreateRenderWindow() {
  cu::AssertError(CheckCapabilities(), "Vulkan not supported", __FILE__,
                  __LINE__);
  render_claimed_ = false;
  load_claimed_ = false;

  PFN_vkCreateInstance pfn_create_instance =
      (PFN_vkCreateInstance)glfwGetInstanceProcAddress(NULL,
                                                       "vkCreateInstance");
  uint32_t count = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&count);

  VkApplicationInfo app_info{};
  app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.applicationVersion = 1;
  app_info.pApplicationName = "Vulkan window";

  VkInstanceCreateInfo inst_create_info{};
  inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  inst_create_info.pApplicationInfo = &app_info;
  inst_create_info.enabledExtensionCount = count;
  inst_create_info.ppEnabledExtensionNames = extensions;

  VkInstance instance{};
  if (pfn_create_instance(&inst_create_info, nullptr, &instance) ==
      VK_SUCCESS) {
    uint32_t number_supported_devices = 1;
    VkPhysicalDevice physical_devices[1];
    if (vkEnumeratePhysicalDevices(instance, &number_supported_devices,
                                   physical_devices) == VK_SUCCESS) {
      VkPhysicalDeviceProperties device_properties = {};
      vkGetPhysicalDeviceProperties(physical_devices[0], &device_properties);

      std::cout << "Device Name:\t" << device_properties.deviceName
                << std::endl;

      switch (device_properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
          std::cout << "Device Type:\tCPU" << std::endl;
          break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
          std::cout << "Device Type:\tDiscrete GPU" << std::endl;
          break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
          std::cout << "Device Type:\tIntegrated GPU" << std::endl;
          break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
          std::cout << "Device Type:\tVirtual GPU" << std::endl;
          break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
          std::cout << "Device Type:\tOther" << std::endl;
          break;
      }

      std::cout << "Highest Supported Vulkan Version:\t"
                << VersionString(device_properties.apiVersion) << std::endl;

      VkPhysicalDeviceFeatures supported_device_features = {};
      VkPhysicalDeviceFeatures desired_device_features = {};

      vkGetPhysicalDeviceFeatures(physical_devices[0],
                                  &supported_device_features);

      std::cout << "Supports Tesselation Shader Feature:\t"
                << ((supported_device_features.tessellationShader) ? ("Yes")
                                                                   : ("No"))
                << std::endl;

      VkPhysicalDeviceMemoryProperties device_memory_properties = {};

      vkGetPhysicalDeviceMemoryProperties(physical_devices[0],
                                          &device_memory_properties);

      uint64_t local_memory_size = 0;

      for (auto &t : device_memory_properties.memoryTypes)
        local_memory_size +=
            device_memory_properties.memoryHeaps[t.heapIndex].size;

      std::cout << "Total Heaps Size:\t"
                << std::to_string((float)local_memory_size /
                                  (1024 * 1024 * 1024))
                << "GB" << std::endl;

      uint32_t num_queue_families = 0;

      vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0],
                                               &num_queue_families, nullptr);

      std::cout << "Number of Queue Families:\t"
                << std::to_string(num_queue_families) << std::endl;

      std::vector<VkQueueFamilyProperties> queue_family_properties;
      queue_family_properties.resize(num_queue_families);

      vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0],
                                               &num_queue_families,
                                               queue_family_properties.data());

      uint32_t index = 0;
      uint32_t queue_count = 0;
      uint32_t graphic_queue_family_index = -1;
      uint32_t graphic_queue_family_num_queue = 0;
      for (auto &p : queue_family_properties) {
        if (p.queueFlags % 2 == 1) {
          graphic_queue_family_index = index;
          graphic_queue_family_num_queue = p.queueCount;
        }
        queue_count += p.queueCount;
        ++index;
      }

      cu::AssertError(
          graphic_queue_family_index != -1 &&
              graphic_queue_family_num_queue != 0,
          "Cannot find graphical queue family with queues available.", __FILE__,
          __LINE__);

      std::cout << "Total Device Queue Count:\t" << std::to_string(queue_count)
                << std::endl;

      float queue_prio[1]{1.f};
      VkDeviceCreateInfo logical_device_create_info;
      VkDeviceQueueCreateInfo device_queue_create_info;
      device_queue_create_info.sType =
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      device_queue_create_info.flags = 0;
      device_queue_create_info.pNext = nullptr;
      device_queue_create_info.queueFamilyIndex = graphic_queue_family_index;
      device_queue_create_info.queueCount = graphic_queue_family_num_queue;
      device_queue_create_info.pQueuePriorities = queue_prio;

      desired_device_features.tessellationShader = VK_TRUE;
      desired_device_features.geometryShader = VK_TRUE;
      desired_device_features.multiDrawIndirect =
          supported_device_features.multiDrawIndirect;

      logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      logical_device_create_info.flags = 0;
      logical_device_create_info.pNext = nullptr;
      logical_device_create_info.queueCreateInfoCount = 1;
      logical_device_create_info.pQueueCreateInfos = &device_queue_create_info;
      logical_device_create_info.enabledExtensionCount = 0;
      logical_device_create_info.enabledLayerCount = 0;
      logical_device_create_info.ppEnabledExtensionNames = nullptr;
      logical_device_create_info.ppEnabledLayerNames = nullptr;
      logical_device_create_info.pEnabledFeatures = &desired_device_features;

      PFN_vkCreateDevice pfnCreateDevice =
          (PFN_vkCreateDevice)glfwGetInstanceProcAddress(instance,
                                                         "vkCreateDevice");
      VkDevice logical_device[1];
      if (pfnCreateDevice(physical_devices[0], &logical_device_create_info,
                          nullptr, logical_device) == VK_SUCCESS) {
        uint32_t layer_properties_count = -1;
        vkEnumerateDeviceLayerProperties(physical_devices[0],
                                         &layer_properties_count, nullptr);

        std::cout << "Number of Device Layers available to physical device:\t"
                  << std::to_string(layer_properties_count) << std::endl;

        std::vector<VkLayerProperties> layer_properties;
        layer_properties.resize(layer_properties_count);

        vkEnumerateDeviceLayerProperties(physical_devices[0],
                                         &layer_properties_count,
                                         layer_properties.data());

        std::cout << "Device Layers:\n";
        for (auto &p : layer_properties) {
          std::cout << "\tLayer #" << std::endl;
          std::cout << "\t\tName:\t" << p.layerName << std::endl;
          std::cout << "\t\tSpecification Version:\t"
                    << VersionString(p.specVersion) << std::endl;
          std::cout << "\t\tDescription:\t" << p.description << std::endl;
        }

        vkEnumerateInstanceLayerProperties(&layer_properties_count, nullptr);
        std::cout << "Number of Instance Layers available to physical device:\t"
                  << std::to_string(layer_properties_count) << std::endl;

        layer_properties.resize(layer_properties_count);
        vkEnumerateInstanceLayerProperties(&layer_properties_count,
                                           layer_properties.data());

        std::cout << "Instance Layers:\n";
        for (auto &l : layer_properties) {
          std::cout << "\tLayer #" << std::endl;
          std::cout << "\t\tName:\t" << l.layerName << std::endl;
          std::cout << "\t\tSpecification Version:\t"
                    << VersionString(l.specVersion) << std::endl;
          std::cout << "\t\tDescription:\t" << l.description << std::endl;
        }

        uint32_t device_extensions_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_devices[0], nullptr,
                                             &device_extensions_count, nullptr);

        std::vector<VkExtensionProperties> device_extensions;
        device_extensions.resize(device_extensions_count);
        vkEnumerateDeviceExtensionProperties(physical_devices[0], nullptr,
                                             &device_extensions_count,
                                             device_extensions.data());

        std::cout << "Device Extensions:\n";
        for (auto &d : device_extensions)
          std::cout << "\t" << d.extensionName << "("
                    << std::to_string(d.specVersion) << ")\n";

        window_ = nullptr;
        if (glfwGetPhysicalDevicePresentationSupport(
                instance, physical_devices[0], graphic_queue_family_index)) {
          glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
          glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
          if (fullscreen_setting_) {
            auto primary_monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode *mode = glfwGetVideoMode(primary_monitor);
            current_dim_ = {mode->width, mode->height};

            if (windowed_setting_) {
              glfwWindowHint(GLFW_RED_BITS, mode->redBits);
              glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
              glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
              glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            }

            for (int i = 0; i < 8; ++i) {
              window_ =
                  glfwCreateWindow(current_dim_.first, current_dim_.second,
                                   "Fract Editor", primary_monitor, nullptr);
              if (window_) break;
            }
          } else {
            current_dim_ = {g_settings.WindowedWidth(),
                            g_settings.WindowedHeight()};
            for (int i = 0; i < 8; ++i) {
              window_ =
                  glfwCreateWindow(current_dim_.first, current_dim_.second,
                                   "Fract Editor", nullptr, nullptr);
              if (window_) break;
            }
          }

          VkSurfaceKHR surface;
          cu::AssertError(
              glfwCreateWindowSurface(
                  instance, static_cast<GLFWwindow *>(window_), NULL, &surface),
              "Failed surface creation", __FILE__, __LINE__);
        }
      }
    }
  }
}

std::string VlWindow::VersionString(uint32_t version_bitmask) {
  char versio_string[128];

  uint32_t major_api_version = version_bitmask >> 22;
  uint32_t minor_api_version = ((version_bitmask << 10) >> 10) >> 12;
  uint32_t patch_api_version = (version_bitmask << 20) >> 20;
  sprintf_s(versio_string, 128, "%d.%d.%d", static_cast<int>(major_api_version),
            static_cast<int>(minor_api_version),
            static_cast<int>(patch_api_version));

  return versio_string;
}

}  // namespace lib_graphics
