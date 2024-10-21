#ifndef XLOG_H
#define XLOG_H

#include <ctime>        // time_t, tm, localtime_s
#include <cstddef>      // size_t
#include <atomic>
#include <mutex>
#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace xlog
{

enum Level : unsigned char
{
    INFO = 0x01,
    ATTENTION = 0x02,
    WARNING = 0x04,
    ERROR = 0x08,
    FATAL = 0x10
};

constexpr unsigned char ALL_LEVEL = INFO | ATTENTION | WARNING | ERROR | FATAL;

inline time_t getCurrentTime()
{
    time_t rslt = 0;
    ::time(&rslt);

    return rslt;
}

inline std::string timeToString(time_t time)
{
    tm lt;
    ::localtime_s(&lt, &time);

    char buffer[32] = {};
    ::strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", &lt);

    return buffer;
}

inline time_t stringToTime(const std::string& str)
{
    tm lt;
    int year = std::atoi(str.substr(0, 4).c_str());
    int month = std::atoi(str.substr(5, 2).c_str());
    int day = std::atoi(str.substr(8, 2).c_str());
    int hour = std::atoi(str.substr(11, 2).c_str());
    int minute = std::atoi(str.substr(14, 2).c_str());
    int second = std::atoi(str.substr(17, 2).c_str());
    lt.tm_year = year - 1900;
    lt.tm_mon = month - 1;
    lt.tm_mday = day;
    lt.tm_hour = hour;
    lt.tm_min = minute;
    lt.tm_sec = second;

    return ::mktime(&lt);
}

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

struct TimeRange
{
    TimeRange() = default;

    TimeRange(time_t start, time_t end) :
        start(start), end(end)
    {}

    TimeRange(const std::string& start, const std::string& end) :
        start(stringToTime(start)), end(stringToTime(end))
    {}

    bool isValid() const
    {
        return start <= end || start == 0 || end == 0;
    }

    bool contains(time_t time) const
    {
        return start <= time && time <= end;
    }

    bool contains(const std::string& str) const
    {
        return contains(stringToTime(str));
    }

    time_t start = 0;
    time_t end = 0;
};

constexpr TimeRange ALL_TIME = TimeRange();

class XLog
{
public:
    static XLog& getInstance()
    {
        static XLog instance;
        return instance;
    }

    void bindOutStream(std::ostream& stream)
    {
        unbindStream();

        mtx_.lock();
        outStream_ = &stream;
        mtx_.unlock();
    }

    void bindFileStream(const std::string& filename)
    {
        std::ofstream* ofs = new std::ofstream(filename, std::ios::app);

        if (ofs->is_open()) {
            unbindStream();
            bindOutStream(*ofs);
            isFileStream_ = true;
        } else {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

    void unbindStream()
    {
        mtx_.lock();

        if (outStream_ && isFileStream_) {
            dynamic_cast<std::ofstream*>(outStream_)->close();
            delete outStream_;
        }
        outStream_ = nullptr;

        mtx_.unlock();

        isFileStream_ = false;
    }

    void setStreamAttributes(unsigned char levelFilter, TimeRange timeFilter = ALL_TIME,
                             bool hasLevel = true, bool hasTimestamp = true)
    {
        levelFilter_ = levelFilter;
        hasLevel_ = hasLevel;
        hasTimestamp_ = hasTimestamp;

        mtx_.lock();
        timeFilter_ = timeFilter;
        mtx_.unlock();
    }

    void resetStreamAttributes()
    {
        setStreamAttributes(ALL_LEVEL, ALL_TIME, true, true);
    }

    size_t count()
    {
        mtx_.lock();
        size_t rslt = logs_.size();
        mtx_.unlock();

        return rslt;
    }

    bool empty()
    {
        mtx_.lock();
        bool rslt = logs_.empty();
        mtx_.unlock();

        return rslt;
    }

    std::string front(bool hasLevel = true, bool hasTimestamp = true)
    {
        mtx_.lock();
        std::string rslt = logDataToString_(logs_.front(), hasLevel, hasTimestamp);
        mtx_.unlock();

        return rslt;
    }

    std::string back(bool hasLevel = true, bool hasTimestamp = true)
    {
        mtx_.lock();
        std::string rslt = logDataToString_(logs_.back(), hasLevel, hasTimestamp);
        mtx_.unlock();

        return rslt;
    }

    void push(Level level, const std::string& message)
    {
        mtx_.lock();

        time_t time = getCurrentTime();
        logs_.push_back(LogData(level, time, message));
        if (outStream_ && (levelFilter_ & level) &&
            (timeFilter_.isValid() && timeFilter_.contains(time)))
            *outStream_ << logDataToString_(logs_.back(), hasLevel_, hasTimestamp_) << std::endl;

        mtx_.unlock();
    }

    void popFront()
    {
        mtx_.lock();
        logs_.pop_front();
        mtx_.unlock();
    }

    void popBack()
    {
        mtx_.lock();
        logs_.pop_back();
        mtx_.unlock();
    }

    void clear()
    {
        mtx_.lock();
        logs_.clear();
        mtx_.unlock();
    }

    void out(std::ostream& os, unsigned char levelFilter, TimeRange timeFilter = ALL_TIME,
             bool hasLevel = true, bool hasTimestamp = true)
    {
        mtx_.lock();
        std::list<LogData> copy = logs_;
        mtx_.unlock();

        for (const auto& data : copy) {
            if ((levelFilter & data.level) &&
                (timeFilter.isValid() && timeFilter.contains(data.time)))
                os << logDataToString_(data, hasLevel, hasTimestamp) << std::endl;
        }
    }

    void out(const std::string& filename, unsigned char levelFilter,
             TimeRange timeFilter = ALL_TIME, bool hasLevel = true, bool hasTimestamp = true)
    {
        std::ofstream ofs(filename, std::ios::app);

        if (ofs.is_open()) {
            out(ofs, levelFilter, timeFilter, hasLevel, hasTimestamp);
            ofs.close();
        } else {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

private:
    struct LogData
    {
        LogData(Level level, time_t time, const std::string& message) :
            level(level), time(time), message(message) 
        {}

        Level level = INFO;
        time_t time = 0;
        std::string message;
    };

    XLog() :
        isFileStream_(false), hasLevel_(true), hasTimestamp_(true), levelFilter_(ALL_LEVEL)
    {}

    ~XLog()
    {
        unbindStream();
    }

    XLog(const XLog& other) = delete;

    XLog& operator=(const XLog& other) = delete;

    static std::string logDataToString_(const LogData& data,
                                        bool hasLevel = true, bool hasTimestamp = true)
    {
        std::string rslt = data.message;

        if (hasLevel)
            rslt = levelToString(data.level) + " " + rslt;

        if (hasTimestamp)
            rslt = timeToString(data.time) + " " + rslt;

        return rslt;
    }

    std::atomic<bool> isFileStream_;
    std::atomic<bool> hasLevel_;
    std::atomic<bool> hasTimestamp_;
    std::atomic<unsigned char> levelFilter_;
    TimeRange timeFilter_;
    std::ostream* outStream_ = nullptr;
    std::mutex mtx_;
    std::list<LogData> logs_;
};

inline void bindOutStream(std::ostream& stream)
{
    XLog::getInstance().bindOutStream(stream);
}

inline void bindFileStream(const std::string& filename)
{
    XLog::getInstance().bindFileStream(filename);
}

inline void unbindStream()
{
    XLog::getInstance().unbindStream();
}

inline void setStreamAttributes(unsigned char levelFilter, TimeRange timeFilter = ALL_TIME,
                                bool hasLevel = true, bool hasTimestamp = true)
{
    XLog::getInstance().setStreamAttributes(levelFilter, timeFilter, hasLevel, hasTimestamp);
}

inline size_t count()
{
    return XLog::getInstance().count();
}

inline bool empty()
{
    return XLog::getInstance().empty();
}

inline std::string front(bool hasLevel = true, bool hasTimestamp = true)
{
    return XLog::getInstance().front(hasLevel, hasTimestamp);
}

inline std::string back(bool hasLevel = true, bool hasTimestamp = true)
{
    return XLog::getInstance().back(hasLevel, hasTimestamp);
}

inline void push(Level level, const std::string& message)
{
    XLog::getInstance().push(level, message);
}

inline void popFront()
{
    XLog::getInstance().popFront();
}

inline void popBack()
{
    XLog::getInstance().popBack();
}

inline void clear()
{
    XLog::getInstance().clear();
}

inline void out(std::ostream& os, unsigned char LevelFilter, TimeRange timeFilter = ALL_TIME,
                bool hasLevel = true, bool hasTimestamp = true)
{
    XLog::getInstance().out(os, LevelFilter, timeFilter, hasLevel, hasTimestamp);
}

inline void out(const std::string& filename, unsigned char LevelFilter,
                TimeRange timeFilter = ALL_TIME, bool hasLevel = true, bool hasTimestamp = true)
{
    XLog::getInstance().out(filename,  LevelFilter, timeFilter, hasLevel, hasTimestamp);
}

}

#endif // !XLOG_H
