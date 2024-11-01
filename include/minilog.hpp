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
constexpr uchar OUT_FLAG_ALL = 0xFF;

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

class MLog
{
public:
    MLog() :
        outFlagOfOutStream_(OUT_FLAG_ALL), outFlagOfFileStream_(OUT_FLAG_ALL),
        levelFilterOfOutStream_(LEVEL_ALL), levelFilterOfFileStream_(LEVEL_ALL)
    {}

    MLog(MLog&& other) noexcept :
        outFlagOfOutStream_(other.outFlagOfOutStream_.load()),
        outFlagOfFileStream_(other.outFlagOfFileStream_.load()),
        levelFilterOfOutStream_(other.levelFilterOfOutStream_.load()),
        levelFilterOfFileStream_(other.levelFilterOfFileStream_.load())
    {
        outStream_ = other.outStream_;
        fileStream_ = other.fileStream_;
        other.outStream_ = nullptr;
        other.fileStream_ = nullptr;
    }

    MLog(const MLog& other) = delete;

    ~MLog()
    {
        unbindOutStream();
        unbindFileStream();
    }

    MLog& operator=(const MLog& other) = delete;

    void bindOutStream(std::ostream& stream, uchar outFlag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        outFlagOfOutStream_ = outFlag;
        levelFilterOfOutStream_ = levelFilter;

        mtx_.lock();
        outStream_ = &stream;
        mtx_.unlock();
    }

    void bindFileStream(const std::string& filename, uchar outFlag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        outFlagOfFileStream_ = outFlag;
        levelFilterOfFileStream_ = levelFilter;

        std::ofstream* ofs = new std::ofstream(filename, std::ios::app);

        if (ofs->is_open()) {
            mtx_.lock();
            fileStream_ = ofs;
            mtx_.unlock();
        } else {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

    void unbindOutStream()
    {
        mtx_.lock();
        outStream_ = nullptr;
        mtx_.unlock();
    }

    void unbindFileStream()
    {
        mtx_.lock();

        if (fileStream_) {
            fileStream_->close();
            delete fileStream_;
            fileStream_ = nullptr;
        }

        mtx_.unlock();
    }

    void setOutStreamAttributes(uchar outFlag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        outFlagOfOutStream_ = outFlag;
        levelFilterOfOutStream_ = levelFilter;
    }

    void setFileStreamAttributes(uchar outFlag = OUT_FLAG_ALL, uchar levelFilter = LEVEL_ALL)
    {
        outFlagOfFileStream_ = outFlag;
        levelFilterOfFileStream_ = levelFilter;
    }

    void log(Level level, const std::string& message)
    {
        std::string curtimeStr = currentTimeString_();
        std::string levelStr = levelToString(level);

        mtx_.lock();

        if (outStream_ && levelFilterOfOutStream_ & level) {
            if (outFlagOfOutStream_ & OUT_WITH_TIMESTAMP)
                *outStream_ << curtimeStr << " ";

            if (outFlagOfOutStream_ & OUT_WITH_LEVEL)
                *outStream_ << levelStr << " ";

            *outStream_ << message << std::endl;
        }

        if (fileStream_ && levelFilterOfFileStream_ & level) {
            if (outFlagOfFileStream_ & OUT_WITH_TIMESTAMP)
                *fileStream_ << curtimeStr << " ";

            if (outFlagOfFileStream_ & OUT_WITH_LEVEL)
                *fileStream_ << levelStr << " ";

            *fileStream_ << message << std::endl;
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

    std::atomic<unsigned char> outFlagOfOutStream_;
    std::atomic<unsigned char> outFlagOfFileStream_;
    std::atomic<unsigned char> levelFilterOfOutStream_;
    std::atomic<unsigned char> levelFilterOfFileStream_;
    std::ostream* outStream_ = nullptr;
    std::ofstream* fileStream_ = nullptr;
    std::mutex mtx_;
};

}

#endif // !MINILOG_HPP
