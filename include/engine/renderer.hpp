#ifndef MIX_ABSTRACT_RENDERER_HPP
#define MIX_ABSTRACT_RENDERER_HPP
#include <SDL2/SDL.h>

#include <stdexcept>
#include <array>

class Renderer {
  protected:
  const int MAX_FRAMES_IN_FLIGHT = 2;
  public:
    bool framebufferResized = false;
    bool minimized = false;
    virtual void init() = 0;
    virtual void drawFrame() = 0;
    virtual void cleanup() = 0;
};
#endif
