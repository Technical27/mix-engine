#include "engine.hpp"

void createObjects(Renderer* renderer) {
  VulkanRenderer* vulkan = (VulkanRenderer*)renderer;
  vulkan->createDescriptorPool(3);
  Pipeline pipeline;
  pipeline.vertShaderPath = "shaders/vert.spv";
  pipeline.fragShaderPath = "shaders/frag.spv";
  vulkan->createGraphicsPipeline(pipeline);
  Object cube = vulkan->createObject("../assets/patch.png");
  Object cube2 = vulkan->createObject("../assets/patch.png");
  Object cube3 = vulkan->createObject("../assets/patch.png");
  cube.updateUBO(vulkan->swapchainExtent);
  cube2.updateUBO(vulkan->swapchainExtent);
  cube3.updateUBO(vulkan->swapchainExtent);
  cube2.ubo.model = glm::translate(cube2.ubo.model, glm::vec3(1.5f, 0.0f, 0.0f));
  cube3.ubo.model = glm::translate(cube3.ubo.model, glm::vec3(-1.5f, 0.0f, 0.0f));
  pipeline.objects.push_back(cube);
  pipeline.objects.push_back(cube2);
  pipeline.objects.push_back(cube3);
  vulkan->pipelines.push_back(pipeline);
}

void Engine::run() {
  Renderer* vulkan = new VulkanRenderer();
  try {
    vulkan->init(createObjects);
    bool quit = false;
    SDL_Event e;
    while (!quit) {
      while (SDL_PollEvent(&e)) {
        switch (e.type) {
          case SDL_WINDOWEVENT:
            switch (e.window.type) {
              case SDL_WINDOWEVENT_RESIZED:
                vulkan->framebufferResized = true;
                break;
              case SDL_WINDOWEVENT_MINIMIZED:
                vulkan->minimized = true;
                break;
              case SDL_WINDOWEVENT_RESTORED:
                vulkan->minimized = false;
                break;
            }
            break;
          case SDL_QUIT:
            quit = true;
            break;
        }
      }
      vulkan->drawFrame();
    }
    vulkan->cleanup();
  }
  catch(EngineException &e) {
    std::cerr << e.what() << std::endl;
  }
}
