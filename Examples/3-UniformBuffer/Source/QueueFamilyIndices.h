#ifndef VULKANLEARNING_QUEUEFAMILYINDICES
#define VULKANLEARNING_QUEUEFAMILYINDICES

#include <cstdint>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentationFamily;

    bool IsComplete() const;
};

#endif // !VULKANLEARNING_QUEUEFAMILYINDICES
