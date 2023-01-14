#include "VulkanApp.h"

int main()
{
    Log::Init();
    VulkanApp App(800, 800);
    App.Run();
}