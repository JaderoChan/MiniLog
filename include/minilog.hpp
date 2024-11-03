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
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <initializer_list>
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

}

// Classes.
namespace mlog
{

class StopWatch
{
    using clock = std::chrono::steady_clock;

public:
    StopWatch() :
        startTime_{clock::now()}
    {}

    // Unit is millisecond.
    ullong elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - startTime_).count();
    }

    void reset() { startTime_ = clock::now(); }

private:
    std::chrono::time_point<clock> startTime_;
};

// The output method.
class Sinks
{
public:
    Sinks(std::ostream& outStream, uchar outFlag = OUT_FLAG_ALL, uchar leveFilter = LEVEL_ALL) :
        os_(&outStream), outFlag_(outFlag), leveFilter_(leveFilter), isOsCreatedByOwn_(false)
    {}

    Sinks(const std::string& filename, uchar outFlag = OUT_FLAG_ALL, uchar leveFilter = LEVEL_ALL) :
        outFlag_(outFlag), leveFilter_(leveFilter), isOsCreatedByOwn_(true), filename_(filename)
    {}

    ~Sinks()
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (isOsCreatedByOwn_ && os_) {
            dynamic_cast<std::ofstream*>(os_)->close();
            delete os_;
            os_ = nullptr;
        }
        isOsCreatedByOwn_ = false;
    }

    Sinks(Sinks&& other) noexcept :
        outFlag_(other.outFlag_.load()),
        leveFilter_(other.leveFilter_.load()),
        isOsCreatedByOwn_(other.isOsCreatedByOwn_.load()),
        os_(other.os_),
        filename_(other.filename_)
    {
        other.os_ = nullptr;
    }

    Sinks(const Sinks& other) :
        outFlag_(other.outFlag_.load()),
        leveFilter_(other.leveFilter_.load()),
        isOsCreatedByOwn_(other.isOsCreatedByOwn_.load()),
        filename_(other.filename_)
    {
        if (!isOsCreatedByOwn_)
            os_ = other.os_;
    }

    Sinks &operator=(const Sinks& other)
    {
        outFlag_ = other.outFlag_.load();
        leveFilter_ = other.leveFilter_.load();
        isOsCreatedByOwn_ = other.isOsCreatedByOwn_.load();
        filename_ = other.filename_;

        if (!isOsCreatedByOwn_)
            os_ = other.os_;

        return *this;
    }

    void setOutFlag(uchar outFlag = OUT_FLAG_ALL) { outFlag_ = outFlag; }

    void setLevelFilter(uchar leveFilter = LEVEL_ALL) { leveFilter_ = leveFilter; }

    void setOutStream(std::ostream& outStream)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (isOsCreatedByOwn_ && os_) {
            dynamic_cast<std::ofstream*>(os_)->close();
            delete os_;
        }
        isOsCreatedByOwn_ = false;

        os_ = &outStream;
    }

    void setOutStream(const std::string& filename)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (isOsCreatedByOwn_ && os_) {
            dynamic_cast<std::ofstream*>(os_)->close();
            delete os_;
        }
        isOsCreatedByOwn_ = true;

        os_ = nullptr;
        filename_ = filename;
    }

private:
    friend class Logger;

    // Whether the current output stream is file output stream created by filename.
    // (Whether take over memory release of output stream.)
    std::atomic<bool> isOsCreatedByOwn_;
    std::atomic<uchar> outFlag_;
    std::atomic<uchar> leveFilter_;
    std::ostream* os_ = nullptr;
    std::string filename_;
    std::mutex mtx_;
};

class Logger
{
public:
    // Get the global instance of Logger.
    static Logger& globalLogger()
    {
        static Logger logger;
        return logger;
    }

    Logger() = default;

    Logger(const Sinks& sinks)
    {
        addSinks(sinks);
    }

    Logger(std::initializer_list<Sinks> sinksList)
    {
        for (auto& var : sinksList)
            addSinks(var);
    }

    ~Logger() = default;

    Logger(const Logger& other) = delete;

    Logger& operator=(const Logger& other) = delete;

    void addSinks(const Sinks& sinks)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        sinks_.push_back(sinks);
    }

    void removeSinks(size_t index)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (index >= sinks_.size())
            throw std::runtime_error("The index is out range.");

        sinks_.erase(sinks_.begin() + index);
    }

    void removeLastSinks()
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (!sinks_.empty())
            sinks_.pop_back();
    }

    Sinks& getSinks(size_t index)
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (index >= sinks_.size())
            throw std::runtime_error("The index is out range.");

        return sinks_.at(index);
    }

    Sinks& getLastSinks()
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (sinks_.empty())
            throw std::runtime_error("No specify member.");

        return sinks_.back();
    }

    template<Level level, typename T>
    void log(const T& message)
    {
        std::string curtimeStr = currentTimeString_();
        std::string levelStr = levelToString(level);

        std::lock_guard<std::mutex> lock(mtx_);

        for (auto& sinks : sinks_) {
            if (!(level & sinks.leveFilter_))
                continue;

            std::lock_guard<std::mutex> sinksLock(sinks.mtx_);
            bool isConsole =  sinks.os_ == &std::cout || sinks.os_ == &std::cerr || sinks.os_ == &std::clog;
            bool isColorize = isConsole && (sinks.outFlag_ & OUT_WITH_COLORIZE);

            std::stringstream ss;

            if (sinks.outFlag_ & OUT_WITH_TIMESTAMP) {
                ss << (isColorize ? "\033[0m\033[1;30m" : "");
                ss << curtimeStr;
                ss << (isColorize ? "\033[0m" : "");
                ss << ' ';
            }

            if (sinks.outFlag_ & OUT_WITH_LEVEL) {
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

            ss << message;

            if (sinks.isOsCreatedByOwn_ && !sinks.os_) {
                sinks.os_ = new std::ofstream(sinks.filename_, std::ios_base::app);
                if (!dynamic_cast<std::ofstream*>(sinks.os_)->is_open())
                    throw std::runtime_error("Failed to open the file: " + sinks.filename_);
            }

            if (sinks.os_)
                *sinks.os_ << ss.str() << std::endl;
        }
    }

    template<Level level, typename T>
    void log(const std::string& message, const T& arg)
    {
        size_t pos = message.find("{}");
        if (pos == std::string::npos) {
            log<level>(message);
        } else {
            std::stringstream ss;
            ss << message.substr(0, pos);
            ss << arg;
            ss << message.substr(pos + 2);
            log<level>(ss.str());
        }
    }

    template<Level level, typename T, typename... Args>
    void log(const std::string& message, const T& arg, Args&&... args)
    {
        size_t pos = message.find("{}");
        if (pos == std::string::npos) {
            log<level>(message);
        } else {
            std::stringstream ss;
            ss << message.substr(0, pos);
            ss << arg;
            ss << message.substr(pos + 2);
            log<level>(ss.str(), std::forward<Args>(args)...);
        }
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

    std::vector<Sinks> sinks_;
    std::mutex mtx_;
};

}

// Faster way.
namespace mlog
{

inline void addSinks(const Sinks& sinks)
{
    Logger::globalLogger().addSinks(sinks);
}

inline void removeSinks(size_t index)
{
    Logger::globalLogger().removeSinks(index);
}

inline void removeLastSinks()
{
    Logger::globalLogger().removeLastSinks();
}

inline Sinks& getSinks(size_t index)
{
    return Logger::globalLogger().getSinks(index);
}

inline Sinks& getLastSinks()
{
    return Logger::globalLogger().getLastSinks();
}

template<Level level, typename T>
void log(const T& message)
{
    Logger::globalLogger().log<level>(message);
}

template<Level level, typename T>
void log(const std::string& message, const T& arg)
{
    Logger::globalLogger().log<level>(message, arg);
}

template<Level level, typename T, typename... Args>
void log(const std::string& message, const T& arg, Args&&... args)
{
    Logger::globalLogger().log<level>(message, arg, std::forward<Args>(args)...);
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
