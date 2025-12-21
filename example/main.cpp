#include <minilog.hpp>

int main()
{
    mlog::addOs("Default", std::clog);

    mlog::debug("This is a debug message");
    mlog::info("This is a info message");
    mlog::warning("This is a warning message");
    mlog::error("This is a error message");
    mlog::fatal("This is a fatal message");

    return 0;
}
