#include "engine/vulkan.hpp"

#define file "src/engine/vulkan/swapchain.cpp"

SwapchainSupportDetails VulkanRenderer::querySwapchainSupport(VkPhysicalDevice device) {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  for (const auto& availableFormat : availableFormats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          return availableFormat;
      }
  }

  return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
  for (const auto& availablePresentMode : availablePresentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
          return availablePresentMode;
      }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  }
  else {
    int width, height;
    SDL_Vulkan_GetDrawableSize(win, &width, &height);
    VkExtent2D actualExtent = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void VulkanRenderer::recreateSwapchain() {
  vkDeviceWaitIdle(device);

  cleanupSwapchain();

  createSwapchain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
}

void VulkanRenderer::cleanupSwapchain() {
  for (auto framebuffer : swapchainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (auto imageView : swapchainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void VulkanRenderer::createSwapchain() {
  SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

  if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchainInfo = {};
  swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainInfo.surface = surface;

  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageFormat = surfaceFormat.format;
  swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainInfo.imageExtent = extent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainInfo.queueFamilyIndexCount = 2;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else {
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  swapchainInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = presentMode;
  swapchainInfo.clipped = VK_TRUE;

  swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
    throw EngineException("failed to create swap chain", file);
  }

  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

  swapchainImageFormat = surfaceFormat.format;
  swapchainExtent = extent;
}

void VulkanRenderer::createImageViews() {
  swapchainImageViews.resize(swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    VkImageViewCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

    imageInfo.image = swapchainImages[i];
    imageInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageInfo.format = swapchainImageFormat;

    imageInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    imageInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageInfo.subresourceRange.baseMipLevel = 0;
    imageInfo.subresourceRange.levelCount = 1;
    imageInfo.subresourceRange.baseArrayLayer = 0;
    imageInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &imageInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
      throw EngineException("failed to create image views!", file);
    }
  }
}
