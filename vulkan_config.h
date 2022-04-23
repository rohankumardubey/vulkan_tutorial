#ifndef VULKAN_CONFIG_H_
#define VULKAN_CONFIG_H_

#include <vector>

#include <vulkan/vulkan_core.h>

class VulkanPresentationContext;

// Centralized logic for app-level Vulkan configuration.
class VulkanConfig {
 public:
  explicit VulkanConfig(const VulkanPresentationContext& presentation_context);
  VulkanConfig(const VulkanConfig&) = delete;
  VulkanConfig& operator=(const VulkanConfig&) = delete;
  ~VulkanConfig();

  // True if the app configuration enables Vulkan validation.
  [[nodiscard]] bool WantValidation() const { return want_validation_; }

  // vkCreateInstance()-friendly list of required Vulkan layers.
  [[nodiscard]] const std::vector<const char*>& RequiredLayers() const {
    return required_layers_;
  }

  // vkCreateInstance()-friendly list of required instance-level Vulkan extensions.
  [[nodiscard]] const std::vector<const char*>& RequiredInstanceExtensions() const {
    return required_instance_extensions_;
  }

  // vkCreateDevice()-friendly list of required device-level Vulkan extensions.
  [[nodiscard]] const std::vector<const char*>& RequiredDeviceExtensions() const {
    return required_device_extensions_;
  }

  [[nodiscard]] const VkPhysicalDeviceFeatures& RequiredFeatures() const {
    return required_features_;
  }

 private:
  const bool want_validation_;
  const std::vector<const char*> required_layers_;
  const std::vector<const char*> required_instance_extensions_;
  const std::vector<const char*> required_device_extensions_;
  const VkPhysicalDeviceFeatures required_features_;
};

#endif  // VULKAN_CONFIG_H_
