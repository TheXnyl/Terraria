#include <vector>

#include <Volk/volk.h>

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);
void DestroyDebugUtilsMessenger(const VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
VkDebugUtilsMessengerEXT createDebugMessenger(const VkInstance instance);
std::vector<const char*> getExtensions();
std::vector<const char*> getLayers();
