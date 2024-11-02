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

#include <ctime>        // time_t, tm, time(), localtime_s(), strftime()
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace mlog
{

using uchar = unsigned char;
using uint = unsigned int;

enum Level : uchar
{
    INFO = 0x01,
    ATTENTION = 0x02,
    WARNING = 0x04,
    ERROR = 0x08,
    FATAL = 0x10
};

enum OutFlag : uchar
{
    OUT_WITH_LEVEL = 0x01,
    OUT_WITH_TIMESTAMP = 0x02
};

constexpr uchar LEVEL_ALL = 0xFF;
constexpr uchar LEVEL_NONE = 0x00;
constexpr uchar OUT_FLAG_ALL = 0xFF;
constexpr uchar OUT_FLAG_NONE = 0x00;

inline std::string levelToString(Level level)
{
    switch (level) {
        case INFO:
            return "[Info]";
        case ATTENTION:
            return "[Attention]";
        case WARNING:
            return "[Warning]";
        case ERROR:
            return "[Error]";
        case FATAL:
            return "[Fatal]";
        default:
            return "";
    }
}

class Sinks
{
public:
    Sinks(std::ostream& outStream, uchar outFlag = OUT_FLAG_ALL, uchar leveFilter = LEVEL_ALL) :
        os_(&outStream), outFlag_(outFlag), leveFilter_(leveFilter), isOsCreatedByOwn_(false)
    {}

    Sinks(const std::string& filename, uchar outFlag = OUT_FLAG_ALL, uchar leveFilter = LEVEL_ALL) :
        os_(new std::ofstream(filename, std::ios_base::app)),
        outFlag_(outFlag), leveFilter_(leveFilter), isOsCreatedByOwn_(true)
    {}

    ~Sinks()
    {
        mtx_.lock();

        if (isOsCreatedByOwn_ && os_) {
            dynamic_cast<std::ofstream*>(os_)->close();
            delete os_;
            os_ = nullptr;
        }
        isOsCreatedByOwn_ = false;

        mtx_.unlock();
    }

    Sinks(Sinks&& other) noexcept :
        outFlag_(other.outFlag_.load()),
        leveFilter_(other.leveFilter_.load()),
        isOsCreatedByOwn_(other.isOsCreatedByOwn_.load()),
        os_(other.os_)
    {
        other.os_ = nullptr;
    }

    Sinks(const Sinks& other) = delete;

    Sinks &operator=(const Sinks& other) = delete;

    void setOutFlag(uchar outFlag = OUT_FLAG_ALL) { outFlag_ = outFlag; }

    void setLevelFilter(uchar leveFilter = LEVEL_ALL) { leveFilter_ = leveFilter; }

    void setOutStream(std::ostream& outStream)
    {
        mtx_.lock();

        if (isOsCreatedByOwn_ && os_) {
            dynamic_cast<std::ofstream*>(os_)->close();
            delete os_;
        }

        os_ = &outStream;
        isOsCreatedByOwn_ = false;

        mtx_.unlock();
    }

    void setOutStream(const std::string& filename)
    {
        mtx_.lock();

        if (isOsCreatedByOwn_ && os_) {
            dynamic_cast<std::ofstream*>(os_)->close();
            delete os_;
        }

        os_ = new std::ofstream(filename, std::ios_base::app);
        isOsCreatedByOwn_ = true;

        mtx_.unlock();
    }

private:
    friend class Logger;

    void out(const std::string& message)
    {
        mtx_.lock();

        if (os_)
            *os_ << message << std::endl;

        mtx_.unlock();
    }

    std::atomic<bool> isOsCreatedByOwn_;
    std::atomic<uchar> outFlag_;
    std::atomic<uchar> leveFilter_;
    std::ostream* os_;
    std::mutex mtx_;
};

class Logger
{
public:
    static Logger& globalLogger()
    {
        static Logger logger;
        return logger;
    }

    Logger() = default;

    ~Logger() = default;

    Logger(const Logger& other) = delete;

    Logger& operator=(const Logger& other) = delete;

    void addSinks(const std::string& nameid, Sinks&& sinks)
    {
        mtx_.lock();

        if (outs_.find(nameid) != outs_.end())
            return;

        outs_.insert( {nameid, std::move(sinks)});

        mtx_.unlock();
    }

    void addSinks(const std::string& nameid, Sinks& sinks)
    {
        addSinks(nameid, std::move(sinks));
    }

    void removeSinks(const std::string& nameid)
    {
        mtx_.lock();

        if (outs_.find(nameid) == outs_.end())
            return;

        outs_.erase(nameid);

        mtx_.unlock();
    }

    void log(Level level, const std::string& message)
    {
        std::string curtimeStr = currentTimeString_();
        std::string levelStr = levelToString(level);

        mtx_.lock();

        for (auto& var : outs_) {
            if (!(level & var.second.leveFilter_))
                continue;

            std::string msg = message;

            if (var.second.outFlag_ & OUT_WITH_LEVEL)
                msg = levelStr + ' ' + msg;

            if (var.second.outFlag_ & OUT_WITH_TIMESTAMP)
                msg = curtimeStr + ' ' + msg;

            var.second.out(msg);
        }

        mtx_.unlock();
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

    std::unordered_map<std::string, Sinks> outs_;
    std::mutex mtx_;
};

inline void addSinks(const std::string& nameid, Sinks&& sinks)
{
    Logger::globalLogger().addSinks(nameid, std::move(sinks));
}

inline void addSinks(const std::string& nameid, Sinks& sinks)
{
    Logger::globalLogger().addSinks(nameid, sinks);
}

inline void removeSinks(const std::string& nameid)
{
    Logger::globalLogger().removeSinks(nameid);
}

inline void log(Level level, const std::string& message)
{
    Logger::globalLogger().log(level, message);
}

}

#endif // !MINILOG_HPP
