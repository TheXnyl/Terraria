#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define WIDTH 1280
#define HEIGHT 720

struct StringArray
{
  int aData;
  const char** data;
};

#ifndef NDEBUG

static VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr)
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(const VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func(instance, debugMessenger, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

static VkDebugUtilsMessengerEXT createDebugMessenger(const VkInstance instance)
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;

  VkDebugUtilsMessengerEXT debugMessenger;
  CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);

  return debugMessenger;
}

#endif

static StringArray getExtensions()
{
  uint32_t aAvailableExtensions;
  vkEnumerateInstanceExtensionProperties(nullptr, &aAvailableExtensions, nullptr);
  VkExtensionProperties* availableExtensions = new VkExtensionProperties[aAvailableExtensions];
  vkEnumerateInstanceExtensionProperties(nullptr, &aAvailableExtensions, availableExtensions);

  uint32_t aGlfwExtensions;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&aGlfwExtensions);
  const char* additionalExtensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

  int aAdditionalExtensions = sizeof(additionalExtensions) / sizeof(additionalExtensions[0]);
  int aExtensions = aGlfwExtensions + aAdditionalExtensions;

  const char** extensions = new const char*[aExtensions];
  std::copy(glfwExtensions, glfwExtensions + aGlfwExtensions, extensions);
  std::copy(additionalExtensions, additionalExtensions + aAdditionalExtensions,
            extensions + aGlfwExtensions);

  for (int i = 0; i < aExtensions; i++)
  {
    bool found = false;
    for (int j = 0; j < (int)aAvailableExtensions; j++)
    {
      if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0)
      {
        found = true;
        break;
      }
    }

    if (!found)
      throw std::runtime_error(std::string("Required extension ") + extensions[i] +
                               " is not supported");
  }

  delete[] availableExtensions;

  return { aExtensions, extensions };
}

#ifndef NDEBUG

static StringArray getLayers()
{
  uint32_t aAvailableLayers;
  vkEnumerateInstanceLayerProperties(&aAvailableLayers, nullptr);
  VkLayerProperties* availableLayers = new VkLayerProperties[aAvailableLayers];
  vkEnumerateInstanceLayerProperties(&aAvailableLayers, availableLayers);

  const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
  int aLayers = sizeof(layers) / sizeof(layers[0]);

  for (int i = 0; i < aLayers; i++)
  {
    bool found = false;
    for (int j = 0; j < (int)aAvailableLayers; j++)
    {
      if (strcmp(layers[i], availableLayers[j].layerName) == 0)
      {
        found = true;
        break;
      }
    }

    if (!found)
      throw std::runtime_error(std::string("Required layer ") + layers[i] + " is not supported");
  }

  const char** heapLayers = new const char*[aLayers];
  std::copy(layers, layers + aLayers, heapLayers);

  delete[] availableLayers;

  return { aLayers, heapLayers };
}

#endif

static GLFWwindow* initGlfw()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

static VkInstance initVulkan()
{
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan";
  appInfo.pEngineName = "No Engine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  StringArray extensions = getExtensions();

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = extensions.aData;
  createInfo.ppEnabledExtensionNames = extensions.data;

#ifndef NDEBUG

  StringArray layers = getLayers();

  createInfo.enabledLayerCount = layers.aData;
  createInfo.ppEnabledLayerNames = layers.data;

#endif

  VkInstance instance;
  vkCreateInstance(&createInfo, nullptr, &instance);

#ifndef NDEBUG
  delete[] layers.data;
#endif
  delete[] extensions.data;

  return instance;
}

static void cleanVulkan(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger)
{
#ifndef NDEBUG
  DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
  vkDestroyInstance(instance, nullptr);
}

static void cleanGlfw(GLFWwindow* window)
{
  glfwDestroyWindow(window);
  glfwTerminate();
}

static void run(GLFWwindow* window)
{
  while (!glfwWindowShouldClose(window))
    glfwPollEvents();
}

int main()
{
  GLFWwindow* window = initGlfw();
  VkInstance instance = initVulkan();

#ifndef NDEBUG
  VkDebugUtilsMessengerEXT debugMessenger = createDebugMessenger(instance);
#else
  VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif

  run(window);

  cleanVulkan(instance, debugMessenger);
  cleanGlfw(window);
}
