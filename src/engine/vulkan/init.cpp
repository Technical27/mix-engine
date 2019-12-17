#include "engine/vulkan.hpp"

#define file "src/engine/vulkan/init.cpp"

void VulkanRenderer::init() {
  SDLinit();
  createInstance();
#ifdef USE_VALIDATION_LAYERS
  setupDebugMessenger();
#endif
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();

  createSwapchain();
  createImageViews();
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipeline("shaders/vert.spv", "shaders/frag.spv");
  createFramebuffers();
  createCommandPool();

  createTextureSampler();

  createDescriptorPool(3);
  Object cube = createObject();
  Object cube2 = createObject();
  Object cube3 = createObject();
  cube.updateUBO(swapchainExtent);
  cube2.updateUBO(swapchainExtent);
  cube3.updateUBO(swapchainExtent);
  cube2.ubo.model = glm::translate(cube2.ubo.model, glm::vec3(1.5f, 0.0f, 0.0f));
  cube3.ubo.model = glm::translate(cube3.ubo.model, glm::vec3(-1.5f, 0.0f, 0.0f));
  pushObject(cube);
  pushObject(cube2);
  pushObject(cube3);

  createCommandBuffers();
  createSyncObjects();
}

#ifdef USE_VALIDATION_LAYERS
void DestroyDebugUtilsMessengerEXT(
  VkInstance instance,
  VkDebugUtilsMessengerEXT debugMessenger,
  const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}
#endif

void VulkanRenderer::cleanup() {
  vkDeviceWaitIdle(device);

  cleanupSwapchain();

  for (auto &obj : objects) {
    obj.destroy(device);
  }

  vkDestroySampler(device, textureSampler, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyDevice(device, nullptr);
#ifdef USE_VALIDATION_LAYERS
  DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  SDL_DestroyWindow(win);
  SDL_Quit();
}

void VulkanRenderer::SDLinit() {
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
    throw EngineException("Failed to init SDL", file);
  }
  win = SDL_CreateWindow(
    "test",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    WIDTH,
    HEIGHT,
    SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
  );
  if (win == nullptr) {
    throw EngineException("Failed to create window", file);
  }
  SDL_SetWindowMinimumSize(win, 640, 480);
}

std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
  uint32_t extensionCount;
  SDL_Vulkan_GetInstanceExtensions(win, &extensionCount, nullptr);
  std::vector<const char*> extensions(extensionCount);
  SDL_Vulkan_GetInstanceExtensions(win, &extensionCount, extensions.data());

#ifdef USE_VALIDATION_LAYERS
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

void VulkanRenderer::createInstance() {
#ifdef USE_VALIDATION_LAYERS
  if (!checkValidationLayerSupport()) {
    throw EngineException("Validation layers were requested, but not supported", file);
  }
#endif

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "test";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_1;

  VkInstanceCreateInfo instanceInfo = {};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();

  instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  instanceInfo.ppEnabledExtensionNames = extensions.data();

#ifdef USE_VALIDATION_LAYERS
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

  instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  instanceInfo.ppEnabledLayerNames = validationLayers.data();

  populateDebugMessengerCreateInfo(debugCreateInfo);
  instanceInfo.pNext = &debugCreateInfo;
#else
  instanceInfo.enabledLayerCount = 0;
  instanceInfo.pNext = nullptr;
#endif

  if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
    throw EngineException("failed to create instance", file);
  }
}

void VulkanRenderer::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

      throw EngineException("failed to create a sync object for a frame", file);
    }
  }
}
