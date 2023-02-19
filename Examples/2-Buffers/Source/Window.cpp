#include "Window.h"

#include "Log.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <utility>

int Window::s_NumInstances = 0;

Window::Window(int Width, int Height, char const *Title) : m_Title{Title}
{
    if (!s_NumInstances)
    {
        if (!glfwInit())
        {
            VKL_CRITICAL("Failed to initialize GLFW!");
            exit(1);
        }
        VKL_INFO("GLFW initialized");
    }
    s_NumInstances++;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no need to create OpenGL context for Vulkan
                                                  // needs more complex logic than needed here
    m_Window = glfwCreateWindow(Width, Height, m_Title, nullptr, nullptr);
    if (!m_Window)
    {
        VKL_CRITICAL("Failed to create GLFW Window!");
        exit(1);
    }

    VKL_INFO("Window {} ({}, {}) created", m_Title, Width, Height);
}

Window::~Window()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        s_NumInstances--;
    }

    if (!s_NumInstances)
    {
        glfwTerminate();
        VKL_INFO("GLFW terminated");
    }
}

Window::Window(Window &&Rhs) noexcept : m_Window{std::exchange(Rhs.m_Window, nullptr)}
{
}

Window &Window::operator=(Window &&Rhs) noexcept
{
    if (this != &Rhs)
    {
        std::swap(m_Window, Rhs.m_Window);
    }

    return *this;
}

GLFWwindow *Window::Get() const
{
    return m_Window;
}

std::pair<int, int> Window::GetFramebufferSize() const
{
    std::pair<int, int> FramebufferSize;
    glfwGetFramebufferSize(m_Window, &FramebufferSize.first, &FramebufferSize.second);
    return FramebufferSize;
}
