#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_config.h"
#include "vulkan_device.h"
#include "vulkan_extension_list.h"
#include "vulkan_layer_list.h"
#include "vulkan_physical_device_list.h"
#include "vulkan_presentation_context.h"

namespace {

constexpr int kWindowWidth = 800;
constexpr int kwindowHeight = 600;

// Dispatches messages from the Vulkan validation layer to an application.
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallbackThunk(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* message_data,
    void *user_data);

class HelloTriangleApplication {
 public:
  HelloTriangleApplication()
    : presentation_context_(), vulkan_config_(presentation_context_) {}

  HelloTriangleApplication(const HelloTriangleApplication&) = delete;
  HelloTriangleApplication& operator=(const HelloTriangleApplication&) = delete;

  ~HelloTriangleApplication() = default;

  void Run() {
    InitVulkan();

    surface_->MainLoop();

    TeardownVulkan();
  }

  void OnVulkanDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                            VkDebugUtilsMessageTypeFlagsEXT message_type,
                            const VkDebugUtilsMessengerCallbackDataEXT* message_data) {
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
        message_type != VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
      std::cerr << "Vulkan validation message: " << message_data->pMessage << std::endl;

      if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::abort();
    }
  }

 private:
  void InitVulkan() {
    VulkanLayerList layers;
    layers.Print();

    VulkanExtensionList extensions;
    extensions.Print();

    CreateVulkanInstance();
    SetupVulkanDebugMessenger();
    surface_ = presentation_context_.CreateSurface(instance_, kWindowWidth, kwindowHeight);
    SelectPhysicalDevice();
  }

  void TeardownVulkan() {
    device_.reset();
    surface_.reset();
    TeardownVulkanDebugMessenger();
    TeardownVulkanInstance();
  }

  void CreateVulkanInstance() {
    VkApplicationInfo application_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Hello Triange",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "No engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_1,
    };

    const std::vector<const char*>& required_layers = vulkan_config_.RequiredLayers();
    const std::vector<const char*>& required_extensions =
        vulkan_config_.RequiredInstanceExtensions();
    VkInstanceCreateInfo instance_create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,  // For MoltenVK.
      .pApplicationInfo = &application_info,
      .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
      .ppEnabledLayerNames = required_layers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
      .ppEnabledExtensionNames = required_extensions.data(),
    };

    // This mildly duplicates SetupVulkanValidationCallback().
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info;
    if (vulkan_config_.WantValidation()) {
      messenger_create_info =  {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = &VulkanDebugCallbackThunk,
        .pUserData = static_cast<void*>(this),
      };
      instance_create_info.pNext = &messenger_create_info;
    }

    VkResult result = vkCreateInstance(&instance_create_info, /*pAllocator=*/nullptr, &instance_);
    if (result != VK_SUCCESS) {
      std::cerr << "vkCreateInstance() failed" << std::endl;
      std::abort();
    }
  }

  void TeardownVulkanInstance() {
    assert(instance_ != VK_NULL_HANDLE);
    vkDestroyInstance(instance_, /*pAllocator=*/nullptr);
  }

  void SetupVulkanDebugMessenger() {
    assert(instance_ != VK_NULL_HANDLE);

    if (!vulkan_config_.WantValidation())
      return;

    // vkCreateDebugUtilsMessengerEXT() isn't available for static linking.
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (!vkCreateDebugUtilsMessengerEXT) {
      std::cerr << "Failed to dynamically locate vkCreateDebugUtilsMessengerEXT()" << std::endl;
      std::abort();
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = &VulkanDebugCallbackThunk,
      .pUserData = static_cast<void*>(this),
    };
    VkResult result = vkCreateDebugUtilsMessengerEXT(
        instance_, &create_info, /*pAllocator=*/nullptr, &debug_messenger_);
    if (result != VK_SUCCESS) {
      std::cerr << "vkCreateDebugUtilsMessengerEXT() failed" << std::endl;
      std::abort();
    }
  }

  void TeardownVulkanDebugMessenger() {
    assert(instance_ != VK_NULL_HANDLE);

    assert(vulkan_config_.WantValidation() == (debug_messenger_ != VK_NULL_HANDLE));
    if (debug_messenger_ == VK_NULL_HANDLE)
      return;

    // vkDestroyDebugUtilsMessengerEXT() isn't available for static linking.
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if (!vkDestroyDebugUtilsMessengerEXT) {
      std::cerr << "Failed to dynamically locate vkDestroyDebugUtilsMessengerEXT()" << std::endl;
      std::abort();
    }
    vkDestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, /*pAllocator=*/nullptr);
  }

  void SelectPhysicalDevice() {
    assert(instance_ != VK_NULL_HANDLE);

    VulkanPhysicalDeviceList devices(instance_);
    devices.Print();

    device_ = devices.CreateLogicalDevice(vulkan_config_, *surface_);
  }

  VulkanPresentationContext presentation_context_;
  VulkanConfig vulkan_config_;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  std::optional<VulkanPresentationSurface> surface_;
  std::optional<VulkanDevice> device_;
};

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallbackThunk(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* message_data,
    void *user_data) {
  assert(user_data);
  HelloTriangleApplication* app = static_cast<HelloTriangleApplication*>(user_data);

  app->OnVulkanDebugMessage(message_severity, message_type, message_data);
  return VK_FALSE;
}


}  // namespace

int main() {
  HelloTriangleApplication app;

  app.Run();
  return 0;
}
