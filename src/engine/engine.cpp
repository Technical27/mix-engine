#include "engine.hpp"

void Engine::run() {
  Renderer* vulkan = new VulkanRenderer();
  try {
    vulkan->init();
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
      if (!vulkan->minimized) vulkan->drawFrame();
    }
    vulkan->cleanup();
  }
  catch(EngineException &e) {
    std::cerr << e.what() << std::endl;
  }
}
