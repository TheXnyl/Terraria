#include <algorithm>
#include <stdexcept>
#include <limits>

#include "swapchain.h"

#include "vk_core.h"

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
  for (const VkPresentModeKHR mode : availablePresentModes)
  {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return mode;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
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

SwapChainDetails getSwapChainDetails(const VkPhysicalDevice physicalDevice,
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

std::vector<VkImageView> createImageViews(const VkDevice logicalDevice,
                                          const std::vector<VkImage> images, VkFormat imageFormat)
{
  std::vector<VkImageView> imageViews(images.size());
  for (uint32_t i = 0; i < images.size(); i++)
  {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = images[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = imageFormat;
    createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                              VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCreateImageView(logicalDevice, &createInfo, nullptr, imageViews.data() + i);
  }

  return imageViews;
}

SwapChainExtended createSwapchain(GLFWwindow* window, VulkanCoreObjects& vulkanCoreObjects)
{
  SwapChainDetails swapChainDetails =
    getSwapChainDetails(vulkanCoreObjects.physicalDevice, vulkanCoreObjects.surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
  VkExtent2D extent = chooseSwapExtent(window, swapChainDetails.capabilities);

  uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
  if (swapChainDetails.capabilities.maxImageCount > 0)
    imageCount = std::min(imageCount, swapChainDetails.capabilities.maxImageCount);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = vulkanCoreObjects.surface;
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

  VkSwapchainKHR swapchain;
  vkCreateSwapchainKHR(vulkanCoreObjects.logicalDevice, &createInfo, nullptr, &swapchain);

  // currently the images themself arent being used but in the future if you use them add them to
  // the struct
  std::vector<VkImage> swapChainImages = vkEnumerate<VkImage>([&](uint32_t* aData, VkImage* data)
  { vkGetSwapchainImagesKHR(vulkanCoreObjects.logicalDevice, swapchain, aData, data); });

  std::vector<VkImageView> swapChainImageViews =
    createImageViews(vulkanCoreObjects.logicalDevice, swapChainImages, surfaceFormat.format);

  return { swapchain, surfaceFormat.format, extent, swapChainImageViews };
}
