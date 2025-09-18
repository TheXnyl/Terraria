#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

struct VulkanCoreObjects;

struct SwapChainExtended
{
  VkSwapchainKHR swapchain;
  VkFormat imageFormat;
  VkExtent2D extent;
  std::vector<VkImageView> imageViews;
};

struct SwapChainDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

SwapChainDetails getSwapChainDetails(const VkPhysicalDevice physicalDevice,
                                     const VkSurfaceKHR surface);

std::vector<VkImageView> createImageViews(const VkDevice logicalDevice,
                                          const std::vector<VkImage> images, VkFormat imageFormat);

SwapChainExtended createSwapchain(GLFWwindow* window, VulkanCoreObjects& vulkanCoreObjects);
