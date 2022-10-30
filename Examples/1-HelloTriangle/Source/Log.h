#ifndef VULAKNLEARNING_LOG
#define VULAKNLEARNING_LOG

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#pragma warning(pop)

class Log
{
public:
    static void Init();

    static std::shared_ptr<spdlog::logger> &GetClientLogger() { return s_Logger; }

private:
    static std::shared_ptr<spdlog::logger> s_Logger;
};

#define VKL_TRACE(...)    ::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VKL_INFO(...)     ::Log::GetClientLogger()->info(__VA_ARGS__)
#define VKL_WARN(...)     ::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VKL_ERROR(...)    ::Log::GetClientLogger()->error(__VA_ARGS__)
#define VKL_CRITICAL(...) ::Log::GetClientLogger()->critical(__VA_ARGS__)

#endif //! VULAKNLEARNING_LOG