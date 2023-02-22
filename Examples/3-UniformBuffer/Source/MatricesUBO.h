#ifndef VULKANLEARNING_MATRICESUBO
#define VULKANLEARNING_MATRICESUBO

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct MatricesUBO
{
    alignas(16) glm::mat4 Model{};
    alignas(16) glm::mat4 ProjectionView{};
};

#endif // !VULKANLEARNING_MATRICESUBO
