#ifndef VULKANLEARNING_WINDOW
#define VULKANLEARNING_WINDOW

#include <utility>

struct GLFWwindow;

class Window
{
public:
    Window(int Width, int Height, char const *Title);
    ~Window();

    Window(Window const &)            = delete;
    Window &operator=(Window const &) = delete;

    Window(Window &&) noexcept;
    Window &operator=(Window &&) noexcept;

    GLFWwindow *Get() const;

    std::pair<int, int> GetFramebufferSize() const;
    int                 GetWidth() const { return GetFramebufferSize().first; }
    int                 GetHeight() const { return GetFramebufferSize().second; }

    char const *GetTitle() const { return m_Title; }

private:
    GLFWwindow *m_Window = nullptr;
    char const *m_Title  = nullptr;
    static int  s_NumInstances;
};

#endif // !VULKANLEARNING_WINDOW
