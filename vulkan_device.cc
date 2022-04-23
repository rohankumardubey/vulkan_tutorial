#include "vulkan_device.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>

#include <vulkan/vulkan_core.h>

#include "vulkan_config.h"
#include "vulkan_presentation_context.h"
#include "vulkan_physical_device.h"
#include "vulkan_surface_support.h"

namespace {

[[nodiscard]] VkDevice CreateDevice(
    const VulkanConfig& vulkan_config,
    const VulkanSurfaceSupport& surface_support,
    VulkanPhysicalDevice& physical_device) {
  assert(physical_device.VulkanHandle() == surface_support.PhysicalDeviceVulkanHandle());
  assert(surface_support.IsAcceptable());

  VkPhysicalDeviceFeatures required_features{};
  required_features.tessellationShader = true;

  VulkanSurfaceSupport::Queues queues = surface_support.QueueFamilyIndexes();

  std::set<uint32_t> family_indexes = {
      queues.graphics_queue_family_index,
      queues.presentation_queue_family_index,
  };

  const float kQueuePriorities[] = {1.0};

  std::vector<VkDeviceQueueCreateInfo> queue_create_info;
  for (uint32_t family_index : family_indexes) {
    queue_create_info.emplace_back(VkDeviceQueueCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = nullptr,
      .queueFamilyIndex = family_index,
      .queueCount = 1,
      .pQueuePriorities = kQueuePriorities,
    });
  }

  const std::vector<const char*>& required_layers = vulkan_config.RequiredLayers();
  assert(physical_device.HasLayers(required_layers));

  std::vector<const char*> required_extensions = vulkan_config.RequiredDeviceExtensions();
  assert(physical_device.HasExtensions(required_extensions));

  // MoltenVK devices must have the VK_KHR_portability_subset extension enabled.
  // This serves as an acknowledgment that we're using a driver that's not
  // fully Vulkan-compliant.
  static constexpr char kPortabilityExtensionName[] = "VK_KHR_portability_subset";
  if (physical_device.HasExtension({kPortabilityExtensionName}))
    required_extensions.push_back(kPortabilityExtensionName);

  VkDeviceCreateInfo device_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .queueCreateInfoCount = static_cast<uint32_t>(queue_create_info.size()),
    .pQueueCreateInfos = queue_create_info.data(),
    .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
    .ppEnabledLayerNames = required_layers.data(),
    .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
    .ppEnabledExtensionNames = required_extensions.data(),
    .pEnabledFeatures = &required_features,
  };

  VkDevice device = VK_NULL_HANDLE;
  VkResult result = vkCreateDevice(physical_device.VulkanHandle(), &device_create_info,
                                   /*pAllocator=*/nullptr, &device);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDevice() failed" << std::endl;
    std::abort();
  }
  return device;
}

[[nodiscard]] VkSwapchainKHR CreateSwapChain(
    const VulkanSurfaceSupport& surface_support,
    const VulkanPresentationSurface& surface,
    VkDevice logical_device) {
  assert(surface.VulkanHandle() == surface_support.SurfaceVulkanHandle());
  assert(surface_support.IsAcceptable());


  VulkanSurfaceSupport::Queues queues = surface_support.QueueFamilyIndexes();
  bool is_unified_queue =
      (queues.graphics_queue_family_index == queues.presentation_queue_family_index);
  const uint32_t queue_family_indexes[] = {
    queues.graphics_queue_family_index,
    queues.presentation_queue_family_index,
  };

  VkSurfaceFormatKHR surface_format = surface_support.BestFormat();
  VkExtent2D image_extent = surface_support.BestExtentFor(surface.Size());
  VkSwapchainCreateInfoKHR create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .surface = surface.VulkanHandle(),
    .minImageCount = static_cast<uint32_t>(surface_support.BestImageCount()),
    .imageFormat = surface_format.format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent = image_extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = is_unified_queue ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
    .queueFamilyIndexCount = static_cast<uint32_t>(is_unified_queue ? 0 : 2),
    .pQueueFamilyIndices = is_unified_queue ? nullptr : queue_family_indexes,
    .preTransform = surface_support.CurrentTransform(),
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = surface_support.BestMode(),
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,  // TODO(pwnall): Change when recreating.
  };

  VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
  VkResult result = vkCreateSwapchainKHR(logical_device, &create_info, /*pAllocator=*/nullptr, &swap_chain);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateSwapchainKHR() failed" << std::endl;
    std::abort();
  }
  return swap_chain;
}

VkQueue GetGraphicsQueue(const VulkanSurfaceSupport& surface_support, VkDevice logical_device) {
  uint32_t family_index = surface_support.QueueFamilyIndexes().graphics_queue_family_index;

  VkQueue queue = VK_NULL_HANDLE;
  vkGetDeviceQueue(logical_device, family_index, /*queueIndex=*/0, &queue);

  assert(queue != VK_NULL_HANDLE);
  return queue;
}

VkQueue GetPresentationQueue(const VulkanSurfaceSupport& surface_support, VkDevice logical_device) {
  uint32_t family_index = surface_support.QueueFamilyIndexes().graphics_queue_family_index;

  VkQueue queue = VK_NULL_HANDLE;
  vkGetDeviceQueue(logical_device, family_index, /*queueIndex=*/0, &queue);

  assert(queue != VK_NULL_HANDLE);
  return queue;
}

[[nodiscard]] std::vector<VkImage> GetSwapChainImages(
    VkDevice logical_device, VkSwapchainKHR swap_chain) {
  assert(swap_chain != VK_NULL_HANDLE);

  uint32_t count = 0;
  VkResult result = vkGetSwapchainImagesKHR(
      logical_device, swap_chain, &count, /*pSwapchainImages=*/nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetSwapchainImagesKHR() failed to return count" << std::endl;
    std::abort();
  }

  std::vector<VkImage> swap_chain_images(count);
  result = vkGetSwapchainImagesKHR(logical_device, swap_chain, &count, swap_chain_images.data());
  if (result != VK_SUCCESS) {
    std::cerr << "vkGetSwapchainImagesKHR() failed to return list" << std::endl;
    std::abort();
  }

  return swap_chain_images;
}

[[nodiscard]] VkImageView CreateImageView(
    VkFormat image_format, VkDevice logical_device, VkImage image) {
  VkImageViewCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = image_format,
    .components = VkComponentMapping{
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    },
    .subresourceRange = VkImageSubresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    },
  };

  VkImageView image_view = VK_NULL_HANDLE;
  VkResult result = vkCreateImageView(
      logical_device, &create_info, /*pAllocator=*/nullptr, &image_view);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateImageView() failed" << std::endl;
    std::abort();
  }
  return image_view;
}

[[nodiscard]] std::vector<VkImageView> CreateImageViews(
    VkFormat image_format, VkDevice logical_device, const std::vector<VkImage>& images) {
  std::vector<VkImageView> image_views;
  image_views.reserve(images.size());

  for (VkImage image : images)
    image_views.push_back(CreateImageView(image_format, logical_device, image));
  return image_views;
}

}  // namespace


VulkanDevice::VulkanDevice(
    const VulkanConfig& vulkan_config, const VulkanSurfaceSupport& surface_support,
    const VulkanPresentationSurface& surface, VulkanPhysicalDevice& physical_device)
    : device_(CreateDevice(vulkan_config, surface_support, physical_device)),
      swap_chain_(CreateSwapChain(surface_support, surface, device_)),
      swap_chain_format_(surface_support.BestFormat()),
      graphics_queue_(GetGraphicsQueue(surface_support, device_)),
      presentation_queue_(GetPresentationQueue(surface_support, device_)),
      swap_chain_images_(GetSwapChainImages(device_, swap_chain_)),
      swap_chain_image_views_(CreateImageViews(
          swap_chain_format_.format, device_, swap_chain_images_)) {
}

VulkanDevice::VulkanDevice(VulkanDevice&& rhs) noexcept
  : device_(rhs.device_), swap_chain_(rhs.swap_chain_), swap_chain_format_(rhs.swap_chain_format_),
    graphics_queue_(rhs.graphics_queue_), presentation_queue_(rhs.presentation_queue_),
    swap_chain_images_(std::move(rhs.swap_chain_images_)),
    swap_chain_image_views_(std::move(rhs.swap_chain_image_views_)) {
  rhs.device_ = VK_NULL_HANDLE;
  rhs.swap_chain_ = VK_NULL_HANDLE;
  rhs.graphics_queue_ = VK_NULL_HANDLE;
  rhs.presentation_queue_ = VK_NULL_HANDLE;
}

VulkanDevice& VulkanDevice::operator=(VulkanDevice&& rhs) noexcept {
  // Vulkan handles need to be std::swap()ed because releasing can throw.
  std::swap(device_, rhs.device_);
  std::swap(swap_chain_, rhs.swap_chain_);

  // std::swap() is unnecessary because `rhs` doesn't need to be valid for use.
  // `rhs` just needs to be in a good enough shape for its destructor to run.
  swap_chain_format_ = rhs.swap_chain_format_;

  std::swap(graphics_queue_, rhs.graphics_queue_);
  std::swap(presentation_queue_, rhs.presentation_queue_);

  swap_chain_images_ = std::move(rhs.swap_chain_images_);
  swap_chain_image_views_ = std::move(rhs.swap_chain_image_views_);
  return *this;
}

VulkanDevice::~VulkanDevice() {
  if (device_ == VK_NULL_HANDLE) {
    assert(swap_chain_ == VK_NULL_HANDLE);
    assert(swap_chain_image_views_.empty());
    return;
  }


  if (swap_chain_ != VK_NULL_HANDLE) {
    for (VkImageView image_view : swap_chain_image_views_)
      vkDestroyImageView(device_, image_view, /*pAllocator=*/nullptr);
    vkDestroySwapchainKHR(device_, swap_chain_, /*pAllocator=*/nullptr);
  } else {
    assert(swap_chain_image_views_.empty());
  }

  vkDeviceWaitIdle(device_);
  vkDestroyDevice(device_, /*pAllocator=*/nullptr);
}
