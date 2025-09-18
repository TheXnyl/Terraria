#pragma once

#include <fstream>
#include <string>

inline std::string readFile(const char* filepath)
{
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
  if (!file)
    throw std::runtime_error("Failed to open file");

  std::streamsize size = file.tellg();
  file.seekg(0);

  std::string content(size, '\0');
  if (!file.read(content.data(), size))
    throw std::runtime_error("Failed to read file" + std::string(filepath));

  return content;
}
