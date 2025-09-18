#pragma once

#include <vector>

#include <Volk/volk.h>

#include "swapchain.h"

struct VulkanCoreObjects
{
  VkSurfaceKHR surface;
  SwapChainExtended swapchain;

  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;

  VkPipelineLayout pipelineLayout;

  VkDebugUtilsMessengerEXT debugMessenger = nullptr;
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
