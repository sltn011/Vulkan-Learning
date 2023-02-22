#include "Vertex.h"

VkVertexInputBindingDescription Vertex::GetInputBindingDescription()
{
    VkVertexInputBindingDescription InputBindingDescription{};
    InputBindingDescription.binding   = 0;
    InputBindingDescription.stride    = sizeof(Vertex);
    InputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return InputBindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::GetInputAttributeDescriptions()
{
    VkVertexInputAttributeDescription PositionAttribute{};
    PositionAttribute.binding  = 0;
    PositionAttribute.location = 0;
    PositionAttribute.format   = VK_FORMAT_R32G32_SFLOAT;
    PositionAttribute.offset   = offsetof(Vertex, Position);

    VkVertexInputAttributeDescription ColorAttribute{};
    ColorAttribute.binding  = 0;
    ColorAttribute.location = 1;
    ColorAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
    ColorAttribute.offset   = offsetof(Vertex, Color);

    return {PositionAttribute, ColorAttribute};
}
