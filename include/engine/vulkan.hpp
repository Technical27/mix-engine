#ifndef MIX_VULKAN_RENDERER_HPP
#define MIX_VULKAN_RENDERER_HPP
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "engine/renderer.hpp"
#include "engine/exception.hpp"
#include "engine/utils.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <set>
#include <algorithm>

#include <cstring>
#include <cstdint>

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
  }
};

struct Object {
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  VkBuffer vertexBuffer;
  VkBuffer indexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkDeviceMemory indexBufferMemory;
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;
  VkImage textureImage;
  VkImageView textureImageView;
  VkDeviceMemory textureImageMemory;
  std::vector<VkDescriptorSet> descriptorSets;
  UniformBufferObject ubo;

  void destroy (VkDevice device) {
    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
  }

  void freeBuffers(VkDevice device) {
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
      vkDestroyBuffer(device, uniformBuffers[i], nullptr);
      vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
  }

  void updateUBO(VkExtent2D extent) {
    ubo.model = ubo.view = ubo.proj = glm::mat4(1.0f);
    ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), extent.width / (float) extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
  }
};

struct Pipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
  std::string vertShaderPath;
  std::string fragShaderPath;
  std::vector<Object> objects;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class VulkanRenderer : public Renderer {
  private:
    VkInstance instance;
    SDL_Window *win;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;

    std::vector<Pipeline> pipelines;

    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    std::vector<VkImageView> swapchainImageViews;

    VkDescriptorSetLayout descriptorSetLayout;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    int currentFrame = 0;

    VkDescriptorPool descriptorPool;

    const uint32_t HEIGHT = 720;
    const uint32_t WIDTH = 1280;

    void SDLinit();

    std::vector<const char*> getRequiredExtensions();
#ifdef USE_VALIDATION_LAYERS
    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {
      "VK_LAYER_KHRONOS_validation"
    };
    bool checkValidationLayerSupport();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
#endif

    const std::vector<const char*> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    void createCube (Object &c) {
      c.vertices = {
        {{-0.5f, -0.3f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.3f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.3f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.3f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
      };
      c.indices = {
        0, 1, 2, 2, 3, 0
      };
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool deviceIsSuitable(VkPhysicalDevice device);

    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void createVertexBuffer(Object& obj);
    void createIndexBuffer(Object& obj);
    void createUniformBuffers(Object& obj);
    void createDescriptorSets(Object& obj);
    void createTextureImage(Object& obj, std::string fileName);
    void createTextureImageView(Object& obj);

    void updateUniformBuffer(uint32_t currentImage);
    void createDescriptorSetLayout();
    void createDescriptorPool(int size);

    void createBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer& buffer,
      VkDeviceMemory& bufferMemory
    );
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createGraphicsPipeline(Pipeline &pipeline);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createImage(
      uint32_t width,
      uint32_t height,
      VkFormat format,
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkImage& image,
      VkDeviceMemory& imageMemory
    );
    VkImageView createImageView(VkImage image, VkFormat format);
    void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createTextureSampler();
    VkSampler textureSampler;

    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void recreateSwapchain();
    void cleanupSwapchain();
  public:
    VkExtent2D swapchainExtent;
    Object createObject(std::string texturePath) {
      Object obj;
      createCube(obj);
      createVertexBuffer(obj);
      createIndexBuffer(obj);
      createUniformBuffers(obj);
      createTextureImage(obj, texturePath);
      createTextureImageView(obj);
      createDescriptorSets(obj);
      return obj;
    }
    void cleanup();
    void init();
    void drawFrame();

    VulkanRenderer() {};
};
#endif
