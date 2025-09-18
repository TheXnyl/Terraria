#include "vk_debug.h"

#include <iostream>
#include <cstring>

#include "vk_core.h"

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
              [[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

void DestroyDebugUtilsMessenger([[maybe_unused]] const VkInstance instance,
                                [[maybe_unused]] VkDebugUtilsMessengerEXT debugMessenger)
{
#ifdef BUILD_DEBUG
  ((PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT"))(instance, debugMessenger, nullptr);
#endif
}

VkDebugUtilsMessengerEXT createDebugMessenger([[maybe_unused]] const VkInstance instance)
{
  VkDebugUtilsMessengerEXT debugMessenger = nullptr;

#ifdef BUILD_DEBUG
  VkDebugUtilsMessengerCreateInfoEXT ci{};
  ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  ci.pfnUserCallback = debugCallback;
  ((PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT"))(instance, &ci, nullptr, &debugMessenger);
#endif

  return debugMessenger;
}

std::vector<const char*> getExtensions()
{
  std::vector<VkExtensionProperties> availableExtensions =
    vkEnumerate<VkExtensionProperties>([](uint32_t* aData, VkExtensionProperties* data)
  { vkEnumerateInstanceExtensionProperties(nullptr, aData, data); });

  uint32_t glfwCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwCount);

#ifdef BUILD_DEBUG
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

std::vector<const char*> getLayers()
{
  std::vector<const char*> layers{};

#ifdef BUILD_DEBUG
  std::vector<VkLayerProperties> availableLayers =
    vkEnumerate<VkLayerProperties>([](uint32_t* aData, VkLayerProperties* data)
  { vkEnumerateInstanceLayerProperties(aData, data); });

  const char* requiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

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
#endif

  return layers;
}
