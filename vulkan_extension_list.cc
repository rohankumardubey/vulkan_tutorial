#include "vulkan_extension_list.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace {

[[nodiscard]] std::vector<VkExtensionProperties> ListVulkanInstanceExtensions() {
  uint32_t count = 0;
  VkResult result = vkEnumerateInstanceExtensionProperties(
      /*pLayerName=*/nullptr, &count, /*pProperties=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateInstanceExtensionProperties() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkExtensionProperties> extensions(count);
  result = vkEnumerateInstanceExtensionProperties(
      /*pLayerName=*/nullptr, &count, extensions.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateInstanceExtensionProperties() failed to return list" << std::endl;
    std::abort();
  }

  return extensions;
}

[[nodiscard]] std::vector<VkExtensionProperties> ListVulkanDeviceExtensions(
    VkPhysicalDevice physical_device) {
  assert(physical_device != VK_NULL_HANDLE);

  uint32_t count = 0;
  VkResult result = vkEnumerateDeviceExtensionProperties(
      physical_device, /*pLayerName=*/nullptr, &count, /*pProperties=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateDeviceExtensionProperties() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkExtensionProperties> extensions(count);
  result = vkEnumerateDeviceExtensionProperties(
      physical_device, /*pLayerName=*/nullptr, &count, extensions.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkEnumerateDeviceExtensionProperties() failed to return list" << std::endl;
    std::abort();
  }

  return extensions;
}

}  // namespace

VulkanExtensionList::VulkanExtensionList() : extensions_(ListVulkanInstanceExtensions()) {}

VulkanExtensionList::VulkanExtensionList(VkPhysicalDevice physical_device)
    : extensions_(ListVulkanDeviceExtensions(physical_device)) {}

VulkanExtensionList::~VulkanExtensionList() = default;

[[nodiscard]] bool VulkanExtensionList::Contains(std::string_view extension_name) const {
  auto extensions_it = std::find_if(extensions_.begin(), extensions_.end(),
                                    [extension_name](const VkExtensionProperties& extension) {
    return extension_name.compare(extension.extensionName) == 0;
  });
  return extensions_it != extensions_.end();
}

void VulkanExtensionList::Print() const {
  std::cout << extensions_.size() << " supported extensions:\n";
  for (const auto& extension : extensions_)
    std::cout << "  " << extension.extensionName << " version: " << extension.specVersion << "\n";
  std::cout << "\n";
}
