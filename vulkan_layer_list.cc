#include "vulkan_layer_list.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace {

[[nodiscard]] std::vector<VkLayerProperties> ListVulkanInstanceLayers() {
  uint32_t count = 0;
  VkResult result = vkEnumerateInstanceLayerProperties(&count, /*pProperties=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateInstanceLayerProperties() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkLayerProperties> layers(count);
  result = vkEnumerateInstanceLayerProperties(&count, layers.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateInstanceLayerProperties() failed to return list" << std::endl;
    std::abort();
  }

  return layers;
}

[[nodiscard]] std::vector<VkLayerProperties> ListVulkanDeviceLayers(
    VkPhysicalDevice physical_device) {
  assert(physical_device != VK_NULL_HANDLE);

  uint32_t count = 0;
  VkResult result = vkEnumerateDeviceLayerProperties(
      physical_device, &count, /*pProperties=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateDeviceLayerProperties() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkLayerProperties> layers(count);
  result = vkEnumerateDeviceLayerProperties(physical_device, &count, layers.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateDeviceLayerProperties() failed to return list" << std::endl;
    std::abort();
  }

  return layers;
}

}  // namespace

VulkanLayerList::VulkanLayerList() : layers_(ListVulkanInstanceLayers()) {}

VulkanLayerList::VulkanLayerList(VkPhysicalDevice physical_device)
    : layers_(ListVulkanDeviceLayers(physical_device)) {}

VulkanLayerList::~VulkanLayerList() = default;

[[nodiscard]] bool VulkanLayerList::Contains(std::string_view layer_name) const {
  auto layers_it = std::find_if(layers_.begin(), layers_.end(),
                                    [layer_name](const VkLayerProperties& layer) {
    return layer_name.compare(layer.layerName) == 0;
  });
  return layers_it != layers_.end();
}

void VulkanLayerList::Print() const {
  std::cout << layers_.size() << " supported layers:\n";
  for (const auto& layer : layers_)
    std::cout << "  " << layer.layerName << " version: " << layer.specVersion << "\n";
  std::cout << "\n";
}
