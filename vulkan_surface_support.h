#ifndef VULKAN_SURFACE_SUPPORT_H_
#define VULKAN_SURFACE_SUPPORT_H_

#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

#include <vulkan/vulkan_core.h>

class VulkanPhysicalDevice;

// Information about a physical device's usability on a surface.
//
// This instance can be discarded after a VulkanDevice is created.
class VulkanSurfaceSupport {
 public:
  struct Queues {
    uint32_t graphics_queue_family_index;
    uint32_t presentation_queue_family_index;
  };

  explicit VulkanSurfaceSupport(const VulkanPhysicalDevice& physical_device, VkSurfaceKHR surface);

  VulkanSurfaceSupport(const VulkanSurfaceSupport&) = delete;
  VulkanSurfaceSupport& operator=(const VulkanSurfaceSupport&) = delete;
  ~VulkanSurfaceSupport();

  [[nodiscard]] bool IsAcceptable() const;

  // Must only be called if IsAcceptable() returns true.
  [[nodiscard]] VkSurfaceTransformFlagBitsKHR CurrentTransform() const;
  [[nodiscard]] VkSurfaceFormatKHR BestFormat() const;
  [[nodiscard]] VkPresentModeKHR BestMode() const;
  [[nodiscard]] VkExtent2D BestExtentFor(VkExtent2D surface_size) const;
  [[nodiscard]] int BestImageCount() const;
  [[nodiscard]] Queues QueueFamilyIndexes() const;

#if !defined(NDEBUG)
  VkPhysicalDevice PhysicalDeviceVulkanHandle() const {
    assert(physical_device_handle_ != VK_NULL_HANDLE);
    return physical_device_handle_;
  }
  VkSurfaceKHR SurfaceVulkanHandle() const {
    assert(surface_handle_ != VK_NULL_HANDLE);
    return surface_handle_;
  }
#endif  // !defined(NDEBUG)

 private:
  const VkSurfaceCapabilitiesKHR capabilities_;
  const std::vector<VkSurfaceFormatKHR> formats_;
  const std::vector<VkPresentModeKHR> modes_;

#if !defined(NDEBUG)
  const VkPhysicalDevice physical_device_handle_;
  const VkSurfaceKHR surface_handle_;
#endif  // !defined(NDEBUG)

  const std::set<uint32_t> graphics_queue_family_indexes_;

  // The list of the device's queue families that can present on the surface.
  const std::set<uint32_t> presentation_queue_family_indexes_;
};

#endif  // VULKAN_SURFACE_SUPPORT_H_
