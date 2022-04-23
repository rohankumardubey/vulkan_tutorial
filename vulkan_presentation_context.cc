#include "vulkan_presentation_context.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

// Vulkan must be included before GLFW to get Vulkan-specific functionality.
#include <GLFW/glfw3.h>


namespace {

[[nodiscard]] std::vector<const char*> GlfwRequiredVulkanExtensions() {
  glfwInit();

  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  if (!glfw_extensions) {
    std::cerr << "GLFW did not find a working Vulkan implementation" << std::endl;
    std::abort();
  }

  return std::vector<const char*>(glfw_extensions, glfw_extensions + glfw_extension_count);
}

[[nodiscard]] std::vector<const char*> KhrSwapchainExtensionList() {
  static constexpr char kKhrSwapchainExtensionName[] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  return {kKhrSwapchainExtensionName};
}

}  // namespace

struct VulkanPresentationSurface::State {
  GLFWwindow* window = nullptr;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  // The instance associated with `surface_`. null until `surface_` is created.
  VkInstance instance = VK_NULL_HANDLE;
};

VulkanPresentationSurface::VulkanPresentationSurface(std::unique_ptr<State> state)
    : state_(std::move(state)) {
  assert(state_ != nullptr);
}

VulkanPresentationSurface::VulkanPresentationSurface(VulkanPresentationSurface&&) noexcept
    = default;
VulkanPresentationSurface& VulkanPresentationSurface::operator=(VulkanPresentationSurface&&)
    noexcept = default;

VulkanPresentationSurface::~VulkanPresentationSurface() {
  if (!state_)
    return;

  assert(state_->surface != VK_NULL_HANDLE);
  assert(state_->instance != VK_NULL_HANDLE);
  vkDestroySurfaceKHR(state_->instance, state_->surface, /*pAllocator=*/nullptr);

  assert(state_->window != nullptr);
  glfwDestroyWindow(state_->window);
}

VkExtent2D VulkanPresentationSurface::Size() const {
  assert(state_);

  int width = 0, height = 0;
  glfwGetFramebufferSize(state_->window, &width, &height);

  return { .width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height) };
}

VkSurfaceKHR VulkanPresentationSurface::VulkanHandle() const {
  assert(state_);
  assert(state_->surface != VK_NULL_HANDLE);

  return state_->surface;
}

void VulkanPresentationSurface::MainLoop() {
  assert(state_ != nullptr);
  assert(state_->window != nullptr);

  while (!glfwWindowShouldClose(state_->window))
    glfwPollEvents();
}

VulkanPresentationContext::VulkanPresentationContext()
    : required_instance_extensions_(GlfwRequiredVulkanExtensions()),
      required_device_extensions_(KhrSwapchainExtensionList()) {}

VulkanPresentationContext::~VulkanPresentationContext() {
  glfwTerminate();
}

VulkanPresentationSurface VulkanPresentationContext::CreateSurface(VkInstance instance, int width, int height) {
  assert(instance != VK_NULL_HANDLE);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan window", /*monitor=*/nullptr,
                                         /*share=*/nullptr);
  if (window == nullptr) {
    std::cerr << "glfwCreateWindow() failed\n";
    std::abort();
  }

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkResult result = glfwCreateWindowSurface(instance, window, /*allocator=*/nullptr, &surface);
  if (result != VK_SUCCESS) {
    std::cerr << "glfwCreateWindowSurface() failed\n";
    std::abort();
  }

  return VulkanPresentationSurface(std::make_unique<VulkanPresentationSurface::State>(
      VulkanPresentationSurface::State{
          .window = window, .surface = surface, .instance = instance }));
}
