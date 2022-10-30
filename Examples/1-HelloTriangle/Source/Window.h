#ifndef VULKANLEARNING_WINDOW
#define VULKANLEARNING_WINDOW

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

    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }

    char const *GetTitle() const { return m_Title; }

private:
    GLFWwindow *m_Window = nullptr;
    int         m_Width  = 0;
    int         m_Height = 0;
    char const *m_Title  = nullptr;
    static int  s_NumInstances;
};

#endif // !VULKANLEARNING_WINDOW
