// The "MiniLog" library written in c++.
//
// Web: https://github.com/JaderoChan/MiniLog
// You can contact me at: c_dl_cn@outlook.com
//
// MIT License
//
// Copyright (c) 2024 頔珞JaderoChan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MINILOG_HPP
#define MINILOG_HPP

#include <cstddef>  // size_t
#include <ctime>    // time_t, tm, time(), localtime_s(), strftime()
#include <chrono>   // For #StopWatch
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>

// Mini Log namespace.
namespace mlog
{

// Just for the intellisense better show "tip about namespace". :)

}

// Type alias, enum and constants.
namespace mlog
{

using uchar = unsigned char;
using uint = unsigned int;
using llong = long long;
using ullong = unsigned long long;

enum Level : uchar
{
    DEBUG = 0x01,
    INFO = 0x02,
    WARN = 0x04,
    ERROR = 0x08,
    FATAL = 0x10
};

enum OutFlag : uchar
{
    // Whether the output attach log level.
    OUT_WITH_LEVEL = 0x01,
    // Whether the output attach timestamp.
    OUT_WITH_TIMESTAMP = 0x02,
    // Whether colorize the output.
    // Just useful for std::cout, std::cerr, std::clog.
    OUT_WITH_COLORIZE = 0x04
};

constexpr uchar LEVEL_ALL = 0xFF;
constexpr uchar LEVEL_NONE = 0x00;
constexpr uchar OUT_FLAG_ALL = 0xFF;
constexpr uchar OUT_FLAG_NONE = 0x00;

}

// Aux functions.
namespace mlog
{

inline std::string levelToString(Level level)
{
    switch (level) {
        case DEBUG:
            return "[Debug]";
        case INFO:
            return "[Info]";
        case WARN:
            return "[Warn]";
        case ERROR:
            return "[Error]";
        case FATAL:
            return "[Fatal]";
        default:
            return "";
    }
}

template<typename T>
std::string formats(const std::string& fmt, const T& arg)
{
    std::stringstream ss;

    if (fmt.size() < 4) {
        size_t pos = fmt.find("{}");
        if (pos == std::string::npos)
            return fmt;

        ss << fmt.substr(0, pos);
        ss << arg;

        return ss.str() + fmt.substr(pos + 2);
    }

    std::string window(4, '\0');
    for (size_t i = 0; i < fmt.size();) {
        window[0] = fmt[i];
        window[1] = i < fmt.size() - 1 ? fmt[i + 1] : '\0';
        window[2] = i < fmt.size() - 2 ? fmt[i + 2] : '\0';
        window[3] = i < fmt.size() - 3 ? fmt[i + 3] : '\0';

        if (window == "{{}}") {
            ss << "{}";
            i += 4;
            continue;
        }

        if (window[0] == '{' && window[1] == '}') {
            ss << arg;
            return ss.str() + fmt.substr(i + 2);
        } else {
            ss << window[0];
            i += 1;
            continue;
        }
    }

    return ss.str();
}

template<typename T, typename... Args>
std::string formats(const std::string& fmt, const T& arg, Args&&... args)
{
    std::stringstream ss;

    if (fmt.size() < 4) {
        size_t pos = fmt.find("{}");
        if (pos == std::string::npos)
            return fmt;

        ss << fmt.substr(0, pos);
        ss << arg;

        return ss.str() + fmt.substr(pos + 2);
    }

    std::string window(4, '\0');
    for (size_t i = 0; i < fmt.size();) {
        window[0] = fmt[i];
        window[1] = i < fmt.size() - 1 ? fmt[i + 1] : '\0';
        window[2] = i < fmt.size() - 2 ? fmt[i + 2] : '\0';
        window[3] = i < fmt.size() - 3 ? fmt[i + 3] : '\0';

        if (window == "{{}}") {
            ss << "{}";
            i += 4;
            continue;
        }

        if (window[0] == '{' && window[1] == '}') {
            ss << arg;
            return ss.str() + formats(fmt.substr(i + 2), std::forward<Args>(args)...);
        } else {
            ss << window[0];
            i += 1;
            continue;
        }
    }

    return ss.str();
}

}

// Classes.
namespace mlog
{

class StopWatch
{
    using clock = std::chrono::steady_clock;

public:
    StopWatch() :
        startTime_(clock::now())
    {}

// Unit is millisecond.
    ullong elapsed() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - startTime_).count();
    }

    void reset() { startTime_ = clock::now(); }

private:
    std::chrono::time_point<clock> startTime_;
};

class Logger
{
public:
    // Get the global instance of Logger.
    static Logger& globalInstance()
    {
        static Logger instance;
        return instance;
    }

    Logger() = default;

    Logger(std::ostream& os, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        addOs(os, outflag, levelFilter);
    }

    Logger(const std::string& filename, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        addOs(filename, outflag, levelFilter);
    }

    ~Logger()
    {
        for (auto& var : outs_) {
            delete var;
            var = nullptr;
        }
    }

    Logger(const Logger& other) = delete;

    Logger& operator=(const Logger& other) = delete;

    void addOs(std::ostream& os, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        OutStream* os_ = new OutStream(&os, outflag, levelFilter);
        outs_.push_back(os_);
    }

    void addOs(const std::string& filename, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        OutStream* os_ = new FileOutStream(filename, outflag, levelFilter);
        outs_.push_back(os_);
    }

    void removeOs(size_t index)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (index >= outs_.size())
            throw std::runtime_error("The index is out range.");

        delete outs_.at(index);
        outs_.erase(outs_.begin() + index);
    }

    void removeLastOs()
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (!outs_.empty()) {
            delete outs_.back();
            outs_.pop_back();
        }
    }

    void setOsAttribute(size_t index, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (index >= outs_.size())
            throw std::runtime_error("The index is out range.");

        outs_[index]->outflag = outflag;
        outs_[index]->levelFilter = levelFilter;
    }

    void setLastOsAttribute(uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (outs_.empty())
            throw std::runtime_error("No specify member.");

        outs_.back()->outflag = outflag;
        outs_.back()->levelFilter = levelFilter;
    }

    template<Level level, typename T>
    void log(const T& message)
    {
        std::string curtimeStr = currentTimeString_();
        std::string levelStr = levelToString(level);

        std::lock_guard<std::mutex> lock(mtx_);

        for (auto& os : outs_) {
            if (!(level & os->levelFilter))
                continue;

            bool isConsole = os->os == &std::cout || os->os == &std::cerr || os->os == &std::clog;
            bool isColorize = isConsole && (os->outflag & OUT_WITH_COLORIZE);

            std::stringstream ss;

            if (os->outflag & OUT_WITH_TIMESTAMP) {
                ss << (isColorize ? "\033[0m\033[1;30m" : "");
                ss << curtimeStr;
                ss << (isColorize ? "\033[0m" : "");
                ss << ' ';
            }

            if (os->outflag & OUT_WITH_LEVEL) {
                if (isColorize) {
                    switch (level) {
                        case DEBUG:
                            ss << "\033[0m\033[34m";
                            break;
                        case INFO:
                            ss << "\033[0m\033[32m";
                            break;
                        case WARN:
                            ss << "\033[0m\033[33m";
                            break;
                        case ERROR:
                            ss << "\033[0m\033[31m";
                            break;
                        case FATAL:
                            ss << "\033[0m\033[35m";
                            break;
                        default:
                            ss << "\033[0m";
                            break;
                    }
                }

                ss << levelStr;
                ss << (isColorize ? "\033[0m" : "");
                ss << ' ';
            }

            ss << message << std::endl;

            if (os->os)
                *os->os << ss.str();
        }
    }

    template<Level level, typename T>
    void log(const std::string& message, const T& arg)
    {
        log<level>(formats(message, arg));
    }

    template<Level level, typename T, typename... Args>
    void log(const std::string& message, const T& arg, Args&&... args)
    {
        log<level>(formats(message, arg, std::forward<Args>(args)...));
    }

    template<typename T>
    void debug(const T& message) { log<DEBUG>(message); }

    template<typename T, typename... Args>
    void debug(const std::string& message, const T& arg, Args&&... args)
    {
        log<DEBUG>(message, arg, std::forward<Args>(args)...);
    }

    template<typename T>
    void info(const T& message) { log<INFO>(message); }

    template<typename T, typename... Args>
    void info(const std::string& message, const T& arg, Args&&... args)
    {
        log<INFO>(message, arg, std::forward<Args>(args)...);
    }

    template<typename T>
    void warn(const T& message) { log<WARN>(message); }

    template<typename T, typename... Args>
    void warn(const std::string& message, const T& arg, Args&&... args)
    {
        log<WARN>(message, arg, std::forward<Args>(args)...);
    }

    template<typename T>
    void error(const T& message) { log<ERROR>(message); }

    template<typename T, typename... Args>
    void error(const std::string& message, const T& arg, Args&&... args)
    {
        log<ERROR>(message, arg, std::forward<Args>(args)...);
    }

    template<typename T>
    void fatal(const T& message) { log<FATAL>(message); }

    template<typename T, typename... Args>
    void fatal(const std::string& message, const T& arg, Args&&... args)
    {
        log<FATAL>(message, arg, std::forward<Args>(args)...);
    }

private:
    struct OutStream
    {
        OutStream(std::ostream* os, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL) :
            os(os), outflag(outflag), levelFilter(levelFilter)
        {}

        virtual ~OutStream() { os = nullptr; }

        uchar outflag = OUT_FLAG_ALL;
        uchar levelFilter = LEVEL_ALL;
        std::ostream* os = nullptr;
    };

    struct FileOutStream final : public OutStream
    {
        FileOutStream(const std::string& filename, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL) :
            OutStream(new std::ofstream(filename, std::ios_base::app), outflag, levelFilter)
        {
            if (!os || !dynamic_cast<std::ofstream*>(os)->is_open())
                throw std::runtime_error("Failed to open the file: " + filename);
        }

        ~FileOutStream()
        {
            if (os) {
                dynamic_cast<std::ofstream*>(os)->close();
                delete os;
            }
        }
    };

    static std::string currentTimeString_()
    {
        time_t time = 0;
        ::time(&time);

        tm lt;
        ::localtime_s(&lt, &time);

        char buffer[32] = {};
        ::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &lt);

        return buffer;
    }

    std::vector<OutStream*> outs_;
    std::mutex mtx_;
};

}

// Faster way.
namespace mlog
{

inline void addOs(std::ostream& os, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
{
    Logger::globalInstance().addOs(os, outflag, levelFilter);
}

inline void addOs(const std::string& filename, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
{
    Logger::globalInstance().addOs(filename, outflag, levelFilter);
}

inline void removeOs(size_t index)
{
    Logger::globalInstance().removeOs(index);
}

inline void removeLastOs()
{
    Logger::globalInstance().removeLastOs();
}

inline void setOsAttribute(size_t index, uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
{
    Logger::globalInstance().setOsAttribute(index, outflag, levelFilter);
}

inline void setLastOsAttribute(uchar outflag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
{
    Logger::globalInstance().setLastOsAttribute(outflag, levelFilter);
}

template<Level level, typename T>
void log(const T& message)
{
    Logger::globalInstance().log<level>(message);
}

template<Level level, typename T>
void log(const std::string& message, const T& arg)
{
    Logger::globalInstance().log<level>(message, arg);
}

template<Level level, typename T, typename... Args>
void log(const std::string& message, const T& arg, Args&&... args)
{
    Logger::globalInstance().log<level>(message, arg, std::forward<Args>(args)...);
}

template<typename T>
void debug(const T& message) { log<DEBUG>(message); }

template<typename T, typename... Args>
void debug(const std::string& message, const T& arg, Args&&... args)
{
    log<DEBUG>(message, arg, std::forward<Args>(args)...);
}

template<typename T>
void info(const T& message) { log<INFO>(message); }

template<typename T, typename... Args>
void info(const std::string& message, const T& arg, Args&&... args)
{
    log<INFO>(message, arg, std::forward<Args>(args)...);
}

template<typename T>
void warn(const T& message) { log<WARN>(message); }

template<typename T, typename... Args>
void warn(const std::string& message, const T& arg, Args&&... args)
{
    log<WARN>(message, arg, std::forward<Args>(args)...);
}

template<typename T>
void error(const T& message) { log<ERROR>(message); }

template<typename T, typename... Args>
void error(const std::string& message, const T& arg, Args&&... args)
{
    log<ERROR>(message, arg, std::forward<Args>(args)...);
}

template<typename T>
void fatal(const T& message) { log<FATAL>(message); }

template<typename T, typename... Args>
void fatal(const std::string& message, const T& arg, Args&&... args)
{
    log<FATAL>(message, arg, std::forward<Args>(args)...);
}

}

#endif // !MINILOG_HPP
