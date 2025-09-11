#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <limits>

#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr const int WIDTH = 1280;
constexpr const int HEIGHT = 720;

struct VulkanCoreHandles
{
  VkInstance instance;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;

  VkDebugUtilsMessengerEXT debugMessenger = nullptr;
};

struct QueueFamilyIndices
{
  int graphics = -1;
  int present = -1;

  bool complete() { return graphics != -1 && present != -1; }
};

struct SwapChainDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

template <typename T, typename F> std::vector<T> vkEnumerate(F&& func)
{
  uint32_t aData = 0;
  func(&aData, nullptr);

  std::vector<T> result(aData);
  if (aData > 0)
    func(&aData, result.data());
  return result;
}

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

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
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

  VkDebugUtilsMessengerEXT debugMessenger;
  CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);

  return debugMessenger;
}

#endif

static std::vector<const char*> getExtensions()
{
  std::vector<VkExtensionProperties> availableExtensions =
    vkEnumerate<VkExtensionProperties>([](uint32_t* aData, VkExtensionProperties* data)
  { vkEnumerateInstanceExtensionProperties(nullptr, aData, data); });

  uint32_t glfwCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwCount);

#ifndef NDEBUG
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  for (const char* extension : extensions)
  {
    bool found = false;
    for (const VkExtensionProperties& availableExtension : availableExtensions)
    {
      if (strcmp(extension, availableExtension.extensionName) == 0)
      {
        found = true;
        break;
      }
    }
    if (!found)
      throw std::runtime_error(std::string("Required extension ") + extension +
                               " is not supported");
  }

  return extensions;
}

#ifndef NDEBUG

static std::vector<const char*> getLayers()
{
  std::vector<VkLayerProperties> availableLayers =
    vkEnumerate<VkLayerProperties>([](uint32_t* aData, VkLayerProperties* data)
  { vkEnumerateInstanceLayerProperties(aData, data); });

  const char* requiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

  std::vector<const char*> layers;
  for (const char* layer : requiredLayers)
  {
    for (const VkLayerProperties& availableLayer : availableLayers)
    {
      if (strcmp(layer, availableLayer.layerName) == 0)
        goto end;
    }
    throw std::runtime_error(std::string("Required layer ") + layer + " is not supported");

  end:
    layers.push_back(layer);
  }

  return layers;
}

#endif

static QueueFamilyIndices getQueueFamilyIndices(const VkPhysicalDevice physicalDevice,
                                                const VkSurfaceKHR surface)
{
  std::vector<VkQueueFamilyProperties> availableQueueFamilies =
    vkEnumerate<VkQueueFamilyProperties>([&](uint32_t* aData, VkQueueFamilyProperties* data)
  { vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, aData, data); });

  QueueFamilyIndices queueFamilies;
  for (int i = 0; i < (int)availableQueueFamilies.size(); i++)
  {
    const VkQueueFamilyProperties& qf = availableQueueFamilies[i];
    if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      queueFamilies.graphics = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

    if ((bool)presentSupport)
      queueFamilies.present = i;

    if (queueFamilies.complete())
      return queueFamilies;
  }

  throw std::runtime_error("Failed to find required queue families");
}

static SwapChainDetails getSwapChainDetails(const VkPhysicalDevice physicalDevice,
                                            const VkSurfaceKHR surface)
{
  SwapChainDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

  details.formats = vkEnumerate<VkSurfaceFormatKHR>([&](uint32_t* aData, VkSurfaceFormatKHR* data)
  { vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, aData, data); });

  details.presentModes = vkEnumerate<VkPresentModeKHR>([&](uint32_t* aData, VkPresentModeKHR* data)
  { vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, aData, data); });

  return details;
}

static VkPhysicalDevice getPhysicalDevice(VulkanCoreHandles& handles)
{
  std::vector<VkPhysicalDevice> physicalDevices =
    vkEnumerate<VkPhysicalDevice>([&](uint32_t* aData, VkPhysicalDevice* data)
  { vkEnumeratePhysicalDevices(handles.instance, aData, data); });

  const auto isDeviceSuitable = [&](const VkPhysicalDevice physicalDevice) -> bool
  {
    if (!getQueueFamilyIndices(physicalDevice, handles.surface).complete())
      return false;

    SwapChainDetails swapChainDetials = getSwapChainDetails(physicalDevice, handles.surface);
    if (swapChainDetials.formats.empty() || swapChainDetials.presentModes.empty())
      return false;

    return true;
  };

  const auto ratePhysicalDevice = [&](const VkPhysicalDevice physicalDevice) -> int
  {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    int score = 0;

    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      score += 1000;
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
      score += 100;
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
      score += 50;

    score += props.limits.maxImageDimension2D;

    return score;
  };

  int maxScore = 0;
  VkPhysicalDevice maxScorePhysicalDevice = nullptr;

  for (VkPhysicalDevice physicalDevice : physicalDevices)
  {
    if (isDeviceSuitable(physicalDevice))
    {
      int score = ratePhysicalDevice(physicalDevice);
      if (score > maxScore)
      {
        maxScore = score;
        maxScorePhysicalDevice = physicalDevice;
      }
    }
  }

  return maxScorePhysicalDevice;
}

static VkInstance createInstance()
{
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan";
  appInfo.pEngineName = "No Engine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  std::vector<const char*> extensions = getExtensions();

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
  std::vector<const char*> layers = getLayers();
  createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = layers.data();
#endif

  VkInstance instance;
  vkCreateInstance(&createInfo, nullptr, &instance);
  return instance;
}

static VkDevice createLogicalDevice(VulkanCoreHandles& handles)
{
  QueueFamilyIndices queueFamilies = getQueueFamilyIndices(handles.physicalDevice, handles.surface);

  std::vector<uint32_t> uniqueQueueFamilies;
  uniqueQueueFamilies.push_back(queueFamilies.graphics);
  if (queueFamilies.present != queueFamilies.graphics)
    uniqueQueueFamilies.push_back(queueFamilies.present);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriority = 1.0f;
  for (uint32_t family : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = family;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;

  const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  createInfo.enabledExtensionCount = 1;
  createInfo.ppEnabledExtensionNames = deviceExtensions;

  VkDevice device;
  if (vkCreateDevice(handles.physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    throw std::runtime_error("Failed to create logical device");

  return device;
}

static VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
  auto isMatch = [](VkSurfaceFormatKHR f, VkFormat format)
  { return f.format == format && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; };

  const VkFormat preferredFormats[] = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB,
                                        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };

  for (const VkFormat preferredFormat : preferredFormats)
  {
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
    {
      if (isMatch(availableFormat, preferredFormat))
        return availableFormat;
    }
  }

  return availableFormats[0];
}

static VkPresentModeKHR
chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
  for (const VkPresentModeKHR mode : availablePresentModes)
  {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return mode;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    return capabilities.currentExtent;
  else
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D extent;

    extent.width = std::clamp((uint32_t)width, capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height = std::clamp((uint32_t)height, capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);

    return extent;
  }

  throw std::runtime_error("chooseSwapExtent: unreachable code reached");
}

static VkSwapchainKHR createSwapchain(GLFWwindow* window, VulkanCoreHandles& handles)
{
  SwapChainDetails swapChainDetails = getSwapChainDetails(handles.physicalDevice, handles.surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
  VkExtent2D extent = chooseSwapExtent(window, swapChainDetails.capabilities);

  uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
  if (swapChainDetails.capabilities.maxImageCount > 0)
    imageCount = std::min(imageCount, swapChainDetails.capabilities.maxImageCount);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = handles.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  VkSwapchainKHR swapChain;
  vkCreateSwapchainKHR(handles.logicalDevice, &createInfo, nullptr, &swapChain);

  return swapChain;
}

static GLFWwindow* initGlfw()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

static void cleanVulkan(const VulkanCoreHandles& handles)
{
#ifndef NDEBUG
  DestroyDebugUtilsMessengerEXT(handles.instance, handles.debugMessenger, nullptr);
#endif

  vkDestroySwapchainKHR(handles.logicalDevice, handles.swapchain, nullptr);
  vkDestroyDevice(handles.logicalDevice, nullptr);
  vkDestroySurfaceKHR(handles.instance, handles.surface, nullptr);
  vkDestroyInstance(handles.instance, nullptr);
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
  VulkanCoreHandles handles;

  GLFWwindow* window = initGlfw();

  handles.instance = createInstance();
  glfwCreateWindowSurface(handles.instance, window, nullptr, &handles.surface);

  handles.physicalDevice = getPhysicalDevice(handles);
  handles.logicalDevice = createLogicalDevice(handles);
  handles.swapchain = createSwapchain(window, handles);

#ifndef NDEBUG
  handles.debugMessenger = createDebugMessenger(handles.instance);
#else
  handles.debugMessenger = nullptr;
#endif

  QueueFamilyIndices queueFamilies = getQueueFamilyIndices(handles.physicalDevice, handles.surface);

  VkQueue graphicsQueue;
  vkGetDeviceQueue(handles.logicalDevice, queueFamilies.graphics, 0, &graphicsQueue);
  VkQueue presentQueue;
  vkGetDeviceQueue(handles.logicalDevice, queueFamilies.present, 0, &presentQueue);

  run(window);

  cleanVulkan(handles);
  cleanGlfw(window);
}
