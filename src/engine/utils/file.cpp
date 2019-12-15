#include "engine/utils.hpp"
#define File "src/engine/utils/file.cpp"

std::vector<char> readFile (const std::string& filename) {
  std::ifstream file (filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw EngineException("failed to open file", File);
  }

  size_t size = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(size);

  file.seekg(0);
  file.read(buffer.data(), size);

  file.close();
  
  return buffer;
}
