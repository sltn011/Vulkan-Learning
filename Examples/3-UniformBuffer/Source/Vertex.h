#ifndef VULAKNLEARNING_VERTEX
#define VULAKNLEARNING_VERTEX

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetInputBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 2> GetInputAttributeDescriptions();
};

#endif