#include "main.hpp"
#include "engine.hpp"
#include "engine/vulkan.hpp"

int main () {
  Renderer *vulkan = new VulkanRenderer();
  try {
    vulkan->init();
  }
  catch (EngineException &e) {
    std::cout << e.what() << std::endl;
  }
  bool quit = false;
  SDL_Event e;
  while (!quit) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_WINDOWEVENT:
          if (e.window.type == SDL_WINDOWEVENT_RESIZED) {
            vulkan->framebufferResized = true;
          }
          else if (e.window.type == SDL_WINDOWEVENT_MINIMIZED) {
            vulkan->minimized = true;
          }
          else if (e.window.type == SDL_WINDOWEVENT_RESTORED) {
            vulkan->minimized = false;
          }
          break;
        case SDL_QUIT:
          quit = true;
          break;
      }
    }
    if (!vulkan->minimized) vulkan->drawFrame();
  }
  vulkan->cleanup();
  return 0;
}
