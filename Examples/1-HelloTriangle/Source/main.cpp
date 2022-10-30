#include "VulkanApp.h"

int main()
{
    Log::Init();
    VulkanApp App(500, 500);
    App.Run();
}