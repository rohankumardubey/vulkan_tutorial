#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include <cassert>
#include <vector>

#include <vulkan/vulkan_core.h>

class VulkanConfig;
class VulkanPhysicalDevice;
class VulkanPresentationSurface;
class VulkanSurfaceSupport;

class VulkanDevice {
 public:
  // Creates a new logical device connected to the given physical device.
  explicit VulkanDevice(
      const VulkanConfig& vulkan_config, const VulkanSurfaceSupport& surface_support,
      const VulkanPresentationSurface& surface, VulkanPhysicalDevice& physical_device);

  // Moving supported so instances can be returned.
  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice(VulkanDevice &&rhs) noexcept;
  VulkanDevice& operator=(const VulkanDevice&) = delete;
  VulkanDevice& operator=(VulkanDevice &&rhs) noexcept;

  // Blocks until all the currently queued operations on the device complete.
  ~VulkanDevice();

  VkDevice VulkanHandle() const {
    assert(device_ != VK_NULL_HANDLE);
    return device_;
  }

  VkQueue GraphicsQueue() const {
    assert(device_ != VK_NULL_HANDLE);
    assert(graphics_queue_ != VK_NULL_HANDLE);
    return graphics_queue_;
  }
  VkQueue PresentationQueue() const {
    assert(device_ != VK_NULL_HANDLE);
    assert(presentation_queue_ != VK_NULL_HANDLE);
    return presentation_queue_;
  }

 private:
  VkDevice device_;
  VkSwapchainKHR swap_chain_;
  VkSurfaceFormatKHR swap_chain_format_;
  // TODO(costan): Add VkExtent2D swap_chain_extent_;
  VkQueue graphics_queue_;
  VkQueue presentation_queue_;
  std::vector<VkImage> swap_chain_images_;
  std::vector<VkImageView> swap_chain_image_views_;
};

#endif  // VULKAN_DEVICE_H_
