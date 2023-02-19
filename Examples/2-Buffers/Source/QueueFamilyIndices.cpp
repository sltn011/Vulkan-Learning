#include "QueueFamilyIndices.h"

bool QueueFamilyIndices::IsComplete() const
{
    return GraphicsFamily.has_value() && PresentationFamily.has_value();
}
