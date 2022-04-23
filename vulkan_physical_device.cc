#include "vulkan_physical_device.h"

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "vulkan_config.h"
#include "vulkan_extension_list.h"
#include "vulkan_layer_list.h"

namespace {

[[nodiscard]] VkPhysicalDeviceProperties GetDeviceProperties(VkPhysicalDevice device) {
  assert(device != VK_NULL_HANDLE);

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);
  return properties;
}

[[nodiscard]] VkPhysicalDeviceFeatures GetDeviceFeatures(VkPhysicalDevice device) {
  assert(device != VK_NULL_HANDLE);

  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(device, &features);
  return features;
}

[[nodiscard]] VkPhysicalDeviceMemoryProperties GetDeviceMemoryProperties(VkPhysicalDevice device) {
  assert(device != VK_NULL_HANDLE);

  VkPhysicalDeviceMemoryProperties properties;
  vkGetPhysicalDeviceMemoryProperties(device, &properties);
  return properties;
}

[[nodiscard]] std::vector<VkQueueFamilyProperties> GetDeviceQueueFamilies(VkPhysicalDevice device) {
  assert(device != VK_NULL_HANDLE);

  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, /*pProperties=*/nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queue_families.data());
  return queue_families;
}

[[nodiscard]] std::set<uint32_t> GetGraphicsQueueFamilyIndexes(
    const std::vector<VkQueueFamilyProperties>& queue_families) {
  std::set<uint32_t> graphics_queue_family_indexes;
  for (size_t queue_family_index = 0; queue_family_index < queue_families.size();
       ++queue_family_index) {
    if (queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      graphics_queue_family_indexes.insert(queue_family_index);
  }
  return graphics_queue_family_indexes;
}

}  // namespace

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice physical_device_handle)
    : physical_device_(physical_device_handle),
      properties_(GetDeviceProperties(physical_device_handle)),
      features_(GetDeviceFeatures(physical_device_handle)),
      memory_properties_(GetDeviceMemoryProperties(physical_device_handle)),
      queue_families_(GetDeviceQueueFamilies(physical_device_handle)),
      graphics_queue_family_indices_(GetGraphicsQueueFamilyIndexes(queue_families_)) {
  assert(physical_device_handle != VK_NULL_HANDLE);
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanPhysicalDevice&&) noexcept = default;
VulkanPhysicalDevice& VulkanPhysicalDevice::operator=(VulkanPhysicalDevice&&) noexcept = default;

VulkanPhysicalDevice::~VulkanPhysicalDevice() = default;

void VulkanPhysicalDevice::Print() const {
  std::cout << "  " << properties_.deviceName  << " id: " << properties_.deviceID
            << " type: " << properties_.deviceType << " API: " << properties_.apiVersion << "\n";
}

bool VulkanPhysicalDevice::HasRequiredFeatures() const {
  return features_.tessellationShader == VK_TRUE;
}

bool VulkanPhysicalDevice::HasLayers(const std::vector<const char*>& layer_names) const {
  VulkanLayerList device_layers(physical_device_);

  for (const char* layer_name : layer_names) {
    if (!device_layers.Contains(layer_name))
      return false;
  }
  return true;
}

bool VulkanPhysicalDevice::HasExtension(std::string_view extension_name) const {
  // TODO(costan): Cache VulkanExtensionList.
  VulkanExtensionList device_extensions(physical_device_);

  return device_extensions.Contains(extension_name);
}

bool VulkanPhysicalDevice::HasExtensions(const std::vector<const char*>& extension_names) const {
  VulkanExtensionList device_extensions(physical_device_);

  for (const char* extension_name : extension_names) {
    if (!device_extensions.Contains(extension_name))
      return false;
  }
  return true;
}
