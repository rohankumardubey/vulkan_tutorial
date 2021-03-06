#ifndef VULKAN_PHYSICAL_DEVICE_H_
#define VULKAN_PHYSICAL_DEVICE_H_

#include <cassert>
#include <cstddef>
#include <set>
#include <string_view>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "vulkan_extension_list.h"

// Information about a physical device's capabilities.
//
// This instance can be discarded after a VulkanDevice is created.
class VulkanPhysicalDevice {
 public:
  // `physical_device_handle` must not be null.
  explicit VulkanPhysicalDevice(VkPhysicalDevice physical_device_handle);

  // Moving supported so instances can be stored in vectors.
  VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
  VulkanPhysicalDevice(VulkanPhysicalDevice&&) noexcept;
  VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;
  VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&&) noexcept;

  ~VulkanPhysicalDevice();

  void Print() const;

  [[nodiscard]] bool HasRequiredFeatures() const;
  [[nodiscard]] bool HasLayers(const std::vector<const char*>& layer_names) const;
  [[nodiscard]] bool HasExtension(std::string_view extension_name) const;
  [[nodiscard]] bool HasExtensions(const std::vector<const char*>& extension_names) const;

  [[nodiscard]] size_t QueueFamilyCount() const { return queue_families_.size(); }

  // The set is empty on devices that don't have any graphics command queues.
  [[nodiscard]] const std::set<uint32_t> GraphicsQueueFamilyIndices() const {
    assert(physical_device_ != VK_NULL_HANDLE);
    return graphics_queue_family_indices_;
  }

  [[nodiscard]] VkPhysicalDevice VulkanHandle() const {
    assert(physical_device_ != VK_NULL_HANDLE);
    return physical_device_;
  }

 private:
  VkPhysicalDevice physical_device_;
  VkPhysicalDeviceProperties properties_;
  VkPhysicalDeviceFeatures features_;
  VkPhysicalDeviceMemoryProperties memory_properties_;
  std::vector<VkQueueFamilyProperties> queue_families_;

  std::set<uint32_t> graphics_queue_family_indices_;
};

#endif  // VULKAN_PHYSICAL_DEVICE_H_
