#include "vulkan_surface_support.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "vulkan_physical_device.h"

namespace {

[[nodiscard]] VkSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilities(
    const VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface) {
  assert(surface != VK_NULL_HANDLE);

  VkSurfaceCapabilitiesKHR capabilities;
  VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physical_device.VulkanHandle(), surface, &capabilities);
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed" << std::endl;
    std::abort();
  }

  return capabilities;
}

[[nodiscard]] std::vector<VkSurfaceFormatKHR> GetPhysicalDeviceSurfaceFormats(
    const VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface) {
  assert(surface != VK_NULL_HANDLE);

  uint32_t count = 0;
  VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(
      physical_device.VulkanHandle(), surface, &count, /*pSurfaceFormats=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetPhysicalDeviceSurfaceFormatsKHR() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkSurfaceFormatKHR> formats(count);
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device.VulkanHandle(), surface, &count,
                                                formats.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetPhysicalDeviceSurfaceFormatsKHR() failed to return list" << std::endl;
    std::abort();
  }

  return formats;
}

[[nodiscard]] std::vector<VkPresentModeKHR> GetPhysicalDeviceSurfacePresentModes(
    const VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface) {
  assert(surface != VK_NULL_HANDLE);

  uint32_t count = 0;
  VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device.VulkanHandle(), surface, &count, /*pSurfaceFormats=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetPhysicalDeviceSurfacePresentModesKHR() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkPresentModeKHR> modes(count);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device.VulkanHandle(), surface, &count, modes.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetPhysicalDeviceSurfacePresentModesKHR() failed to return list" << std::endl;
    std::abort();
  }

  return modes;
}

std::set<uint32_t> GetPresentationQueueFamilyIndexes(
    const VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface) {
  std::set<uint32_t> presentation_queue_family_indexes;

  VkPhysicalDevice physical_device_handle = physical_device.VulkanHandle();
  size_t queue_family_count = physical_device.QueueFamilyCount();
  for (size_t queue_family_index = 0; queue_family_index < queue_family_count;
       ++queue_family_index) {
    VkBool32 can_present_on_surface = VK_FALSE;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(
        physical_device_handle, static_cast<uint32_t>(queue_family_index), surface,
        &can_present_on_surface);
    if (result != VK_SUCCESS) {
      std::cerr << "vkGetPhysicalDeviceSurfaceSupportKHR() failed";
      std::abort();
    }

    if (can_present_on_surface)
      presentation_queue_family_indexes.insert(queue_family_index);
  }
  return presentation_queue_family_indexes;
}

}  // namespace

VulkanSurfaceSupport::VulkanSurfaceSupport(const VulkanPhysicalDevice& physical_device,
                                           VkSurfaceKHR surface)
    : capabilities_(GetPhysicalDeviceSurfaceCapabilities(physical_device, surface)),
      formats_(GetPhysicalDeviceSurfaceFormats(physical_device, surface)),
      modes_(GetPhysicalDeviceSurfacePresentModes(physical_device, surface)),
#if !defined(NDEBUG)
      physical_device_handle_(physical_device.VulkanHandle()),
      surface_handle_(surface),
#endif  // !defined(NDEBUG)
      graphics_queue_family_indexes_(physical_device.GraphicsQueueFamilyIndices()),
      presentation_queue_family_indexes_(
          GetPresentationQueueFamilyIndexes(physical_device, surface)) {
  assert(physical_device.VulkanHandle() != VK_NULL_HANDLE);
  assert(surface != VK_NULL_HANDLE);
  assert(!physical_device.GraphicsQueueFamilyIndices().empty());
}

VulkanSurfaceSupport::~VulkanSurfaceSupport() = default;

bool VulkanSurfaceSupport::IsAcceptable() const {
  return !formats_.empty() && !modes_.empty() && !graphics_queue_family_indexes_.empty() &&
         !presentation_queue_family_indexes_.empty();
}

VkSurfaceTransformFlagBitsKHR VulkanSurfaceSupport::CurrentTransform() const {
  assert(IsAcceptable());
  return capabilities_.currentTransform;
}

VkSurfaceFormatKHR VulkanSurfaceSupport::BestFormat() const {
  assert(IsAcceptable());
  assert(!formats_.empty());

  auto it = std::find_if(formats_.begin(), formats_.end(), [](VkSurfaceFormatKHR format) {
    if (format.format != VK_FORMAT_B8G8R8A8_SRGB)
      return false;
    if (format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return false;
    return true;
  });
  if (it != formats_.end())
    return *it;

  return formats_[0];
}

VkPresentModeKHR VulkanSurfaceSupport::BestMode() const {
  assert(IsAcceptable());
  assert(!modes_.empty());

  auto it = std::find_if(modes_.begin(), modes_.end(), [](VkPresentModeKHR mode) {
    return mode == VK_PRESENT_MODE_MAILBOX_KHR;
  });
  if (it != modes_.end())
    return *it;

  // The Vulkan spec requires VK_PRESENT_MODE_FIFO_KHR support.
  assert(std::count(modes_.begin(), modes_.end(), VK_PRESENT_MODE_FIFO_KHR) == 1);

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSurfaceSupport::BestExtentFor(VkExtent2D surface_size) const {
  assert(IsAcceptable());

  uint32_t extent_width = std::clamp(surface_size.width, capabilities_.minImageExtent.width,
                                     capabilities_.maxImageExtent.width);
  uint32_t extent_height = std::clamp(surface_size.height, capabilities_.minImageExtent.height,
                                      capabilities_.maxImageExtent.height);
  return { .width = extent_width, .height = extent_height };
}

int VulkanSurfaceSupport::BestImageCount() const {
  assert(IsAcceptable());

  // One slack image reduces the risk of being blocked on driver ops.
  uint32_t image_count = capabilities_.minImageCount + 1;

  if (image_count < capabilities_.maxImageCount && capabilities_.maxImageCount != 0)
    image_count = capabilities_.maxImageCount;

  return static_cast<int>(image_count);
}

VulkanSurfaceSupport::Queues VulkanSurfaceSupport::QueueFamilyIndexes() const {
  assert(IsAcceptable());
  assert(!graphics_queue_family_indexes_.empty());
  assert(!presentation_queue_family_indexes_.empty());

  // Prefer to use the same queue family for graphics and presentation commands.
  //
  // This avoids having to share images across queues.
  for (uint32_t graphics_queue_family_index : graphics_queue_family_indexes_) {
    if (presentation_queue_family_indexes_.count(graphics_queue_family_index)) {
      return {
        .graphics_queue_family_index = graphics_queue_family_index,
        .presentation_queue_family_index = graphics_queue_family_index,
      };
    }
  }

  // No queue family supports both graphics commands and presentation commands
  // for the given device. Fall back to the first queue family in each category.
  return {
    .graphics_queue_family_index = *graphics_queue_family_indexes_.begin(),
    .presentation_queue_family_index = *graphics_queue_family_indexes_.begin(),
  };
}
