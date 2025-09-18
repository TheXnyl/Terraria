#include <cstdint>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <Volk/volk.h>

#include "swapchain.h"
#include "vk_core.h"
#include "utils.h"
#include "vk_debug.h"

constexpr const int WIDTH = 1280;
constexpr const int HEIGHT = 720;

struct QueueFamilyIndices
{
  int graphics = -1;
  int present = -1;

  bool complete() { return graphics != -1 && present != -1; }
};

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

static VkPhysicalDevice getPhysicalDevice(VkInstance instance, VulkanCoreObjects& vulkanCoreObjects)
{
  std::vector<VkPhysicalDevice> physicalDevices =
    vkEnumerate<VkPhysicalDevice>([&](uint32_t* aData, VkPhysicalDevice* data)
  { vkEnumeratePhysicalDevices(instance, aData, data); });

  const auto isDeviceSuitable = [&](const VkPhysicalDevice physicalDevice) -> bool
  {
    if (!getQueueFamilyIndices(physicalDevice, vulkanCoreObjects.surface).complete())
      return false;

    SwapChainDetails swapChainDetails =
      getSwapChainDetails(physicalDevice, vulkanCoreObjects.surface);
    if (swapChainDetails.formats.empty() || swapChainDetails.presentModes.empty())
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

  VkInstanceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ci.pApplicationInfo = &appInfo;
  ci.enabledExtensionCount = (uint32_t)extensions.size();
  ci.ppEnabledExtensionNames = extensions.data();

  std::vector<const char*> layers = getLayers();
  ci.enabledLayerCount = (uint32_t)layers.size();
  ci.ppEnabledLayerNames = layers.data();

  VkInstance instance;
  vkCreateInstance(&ci, nullptr, &instance);

  volkLoadInstance(instance);

  return instance;
}

static VkDevice createLogicalDevice(VulkanCoreObjects& vulkanCoreObjects)
{
  QueueFamilyIndices queueFamilies =
    getQueueFamilyIndices(vulkanCoreObjects.physicalDevice, vulkanCoreObjects.surface);

  std::vector<uint32_t> uniqueQueueFamilies;
  uniqueQueueFamilies.push_back(queueFamilies.graphics);
  if (queueFamilies.present != queueFamilies.graphics)
    uniqueQueueFamilies.push_back(queueFamilies.present);

  std::vector<VkDeviceQueueCreateInfo> queuesCI;
  float queuePriority = 1.0f;
  for (uint32_t family : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCI{};
    queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCI.queueFamilyIndex = family;
    queueCI.queueCount = 1;
    queueCI.pQueuePriorities = &queuePriority;
    queuesCI.push_back(queueCI);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  ci.queueCreateInfoCount = (uint32_t)queuesCI.size();
  ci.pQueueCreateInfos = queuesCI.data();
  ci.pEnabledFeatures = &deviceFeatures;

  const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  ci.enabledExtensionCount = 1;
  ci.ppEnabledExtensionNames = deviceExtensions;

  VkDevice logicalDevice;
  if (vkCreateDevice(vulkanCoreObjects.physicalDevice, &ci, nullptr, &logicalDevice) != VK_SUCCESS)
    throw std::runtime_error("Failed to create logical device");

  volkLoadDevice(logicalDevice);

  return logicalDevice;
}

VkShaderModule createShaderModule(const VkDevice logicalDevice, const std::string& shaderByteCode)
{
  VkShaderModuleCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ci.codeSize = shaderByteCode.size();

  std::vector<char> vectorShaderByteCode(shaderByteCode.begin(), shaderByteCode.end());
  ci.pCode = (const uint32_t*)vectorShaderByteCode.data();

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(logicalDevice, &ci, nullptr, &shaderModule) != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module!");

  return shaderModule;
}

static VkPipelineLayout createGraphicsPipeline(const VkDevice logicalDevice,
                                               const VkExtent2D swapChainExtent)
{
  std::string vertexShaderByteCode =
    readFile("Resources/Shaders/Bin/basic_triangle.fragment.glsl.spv");
  std::string fragmentShaderByteCode =
    readFile("Resources/Shaders/Bin/basic_triangle.vertex.glsl.spv");

  VkShaderModule vertexShaderModule = createShaderModule(logicalDevice, vertexShaderByteCode);
  VkShaderModule fragmenthaderModule = createShaderModule(logicalDevice, fragmentShaderByteCode);

  VkPipelineShaderStageCreateInfo vertexShaderStageCI{};
  vertexShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStageCI.module = vertexShaderModule;
  vertexShaderStageCI.pName = "main";
  vertexShaderStageCI.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo fragmentShaderStageCI{};
  fragmentShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStageCI.module = fragmenthaderModule;
  fragmentShaderStageCI.pName = "main";
  fragmentShaderStageCI.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageCI, fragmentShaderStageCI };

  VkPipelineVertexInputStateCreateInfo vertexInputCI{};
  vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputCI.vertexBindingDescriptionCount = 0;
  vertexInputCI.pVertexBindingDescriptions = nullptr;
  vertexInputCI.vertexAttributeDescriptionCount = 0;
  vertexInputCI.pVertexAttributeDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
  inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyCI.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportStateCI{};
  viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateCI.viewportCount = 1;
  viewportStateCI.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizerCI{};
  rasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerCI.depthClampEnable = VK_FALSE;
  rasterizerCI.rasterizerDiscardEnable = VK_FALSE;
  rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizerCI.lineWidth = 1.0f;
  rasterizerCI.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizerCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizerCI.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisamplingCI{};
  multisamplingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingCI.sampleShadingEnable = VK_FALSE;
  multisamplingCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisamplingCI.minSampleShading = 1.0f;
  multisamplingCI.pSampleMask = nullptr;
  multisamplingCI.alphaToCoverageEnable = VK_FALSE;
  multisamplingCI.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendingCI{};
  colorBlendingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingCI.logicOpEnable = VK_FALSE;
  colorBlendingCI.attachmentCount = 1;
  colorBlendingCI.pAttachments = &colorBlendAttachment;

  VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamicStateCI{};
  dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateCI.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
  dynamicStateCI.pDynamicStates = dynamicStates;

  VkPipelineLayoutCreateInfo pipelineLayoutCI{};
  pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

  VkPipelineLayout pipelineLayout;
  if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCI, nullptr, &pipelineLayout) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout!");

  vkDestroyShaderModule(logicalDevice, fragmenthaderModule, nullptr);
  vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);

  return pipelineLayout;
}

static GLFWwindow* initGlfw()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  return glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

static void cleanVulkan(VkInstance instance, const VulkanCoreObjects& vulkanCoreObjects)
{
  DestroyDebugUtilsMessenger(instance, vulkanCoreObjects.debugMessenger);

  vkDestroyPipelineLayout(vulkanCoreObjects.logicalDevice, vulkanCoreObjects.pipelineLayout,
                          nullptr);

  for (const VkImageView imageView : vulkanCoreObjects.swapchain.imageViews)
    vkDestroyImageView(vulkanCoreObjects.logicalDevice, imageView, nullptr);

  vkDestroySwapchainKHR(vulkanCoreObjects.logicalDevice, vulkanCoreObjects.swapchain.swapchain,
                        nullptr);

  vkDestroyDevice(vulkanCoreObjects.logicalDevice, nullptr);
  vkDestroySurfaceKHR(instance, vulkanCoreObjects.surface, nullptr);
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

  volkInitialize();

  VulkanCoreObjects vulkanCoreObjects;
  VkInstance instance = createInstance();

  glfwCreateWindowSurface(instance, window, nullptr, &vulkanCoreObjects.surface);

  vulkanCoreObjects.debugMessenger = createDebugMessenger(instance);
  vulkanCoreObjects.physicalDevice = getPhysicalDevice(instance, vulkanCoreObjects);
  vulkanCoreObjects.logicalDevice = createLogicalDevice(vulkanCoreObjects);

  vulkanCoreObjects.swapchain = createSwapchain(window, vulkanCoreObjects);
  vulkanCoreObjects.pipelineLayout =
    createGraphicsPipeline(vulkanCoreObjects.logicalDevice, vulkanCoreObjects.swapchain.extent);

  QueueFamilyIndices queueFamilies =
    getQueueFamilyIndices(vulkanCoreObjects.physicalDevice, vulkanCoreObjects.surface);
  VkQueue graphicsQueue;
  vkGetDeviceQueue(vulkanCoreObjects.logicalDevice, queueFamilies.graphics, 0, &graphicsQueue);
  VkQueue presentQueue;
  vkGetDeviceQueue(vulkanCoreObjects.logicalDevice, queueFamilies.present, 0, &presentQueue);

  run(window);

  cleanVulkan(instance, vulkanCoreObjects);
  cleanGlfw(window);
}
