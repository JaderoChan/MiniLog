// The "xlog" library written in c++.
//
// Web: https://github.com/JaderoChan/XLog
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

// Prevents multiple inclusion.
#ifndef XLOG_HPP
#define XLOG_HPP

// Includes.
#include <cstddef>  // size_t
#include <cstdlib>  // atoi()
#include <ctime>    // time_t, tm, time(), localtime_s(), strftime(), mktime()
#include <atomic>
#include <mutex>
#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <cassert>
#include <stdexcept>

// XLog namespace.
namespace xlog
{

// Just show XLog namespace on tooltip not is "Enum and constants." so on.

}

// Enum and constants.
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

enum LocalFlag : unsigned int
{
    NUM,
    NUM_PADDING,
    EN,
    EN_SHORT,
    CN,
    JP,
    KR
};

enum ErrorType : unsigned char
{
    INVALID_YEAR,
    INVALID_MONTH,
    INVALID_DAY,
    INVALID_HOUR,
    INVALID_MINUTE,
    INVALID_SECOND,
    INVALID_WEEKDAY,
    INVALID_YEARDAY,
    INVALID_UTC_OFFSET,
    INVALID_DATETIME,
    FAILED_OPEN_FILE
};

constexpr unsigned int MINUTE_SECOND = 60;
constexpr unsigned int HOUR_SECOND = 60 * MINUTE_SECOND;
constexpr unsigned int DAY_SECOND = 24 * HOUR_SECOND;

constexpr unsigned char ALL_LEVEL = INFO | ATTENTION | WARNING | ERROR | FATAL;

// Month string arrays.
// Just for the code block can be fold.
namespace
{

constexpr const char *MONTH_STR_EN[] = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

constexpr const char *MONTH_STR_EN_SHORT[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

constexpr const char *MONTH_STR_CN[] = {
    u8"一月",
    u8"二月",
    u8"三月",
    u8"四月",
    u8"五月",
    u8"六月",
    u8"七月",
    u8"八月",
    u8"九月",
    u8"十月",
    u8"十一月·",
    u8"十二月"
};

constexpr const char *MONTH_STR_JP[] = {
    u8"いちがつ",
    u8"にがつ",
    u8"さんがつ",
    u8"しがつ",
    u8"ごがつ",
    u8"ろくがつ",
    u8"しちがつ",
    u8"はちがつ",
    u8"くがつ",
    u8"じゅうがつ",
    u8"じゅういちがつ",
    u8"じゅうにがつ"
};

constexpr const char *MONTH_STR_KR[] = {
    u8"일월",
    u8"이월",
    u8"삼월",
    u8"사월",
    u8"오월",
    u8"유월",
    u8"칠월",
    u8"팔월",
    u8"구월",
    u8"시월",
    u8"십일월",
    u8"십이월"
};

}

// Weekday string arrays.
// Just for the code block can be fold.
namespace
{

constexpr const char *WEEK_STR_EN[] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

constexpr const char *WEEK_STR_EN_SHORT[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

constexpr const char *WEEK_STR_CN[] = {
    u8"星期日",
    u8"星期一",
    u8"星期二",
    u8"星期三",
    u8"星期四",
    u8"星期五",
    u8"星期六"
};

constexpr const char *WEEK_STR_JP[] = {
    u8"日曜日",
    u8"月曜日",
    u8"火曜日",
    u8"水曜日",
    u8"木曜日",
    u8"金曜日",
    u8"土曜日"
};

constexpr const char *WEEK_STR_KR[] = {
    u8"일요일",
    u8"월요일",
    u8"화요일",
    u8"수요일",
    u8"목요일",
    u8"금요일",
    u8"토요일"
};

}

}

// Error handle.
namespace xlog
{

inline std::string getErrorMessage(ErrorType et)
{
    switch (et) {
        case INVALID_YEAR:
            return "The invalid year.";
        case INVALID_MONTH:
            return "The invalid month.";
        case INVALID_DAY:
            return "The invalid day.";
        case INVALID_HOUR:
            return "The invalid hour.";
        case INVALID_MINUTE:
            return "The invalid minute.";
        case INVALID_SECOND:
            return "The invalid second.";
        case INVALID_WEEKDAY:
            return "The invalid week day.";
        case INVALID_YEARDAY:
            return "The invalid year day.";
        case INVALID_UTC_OFFSET:
            return "The invalid UTC offset.";
        case INVALID_DATETIME:
            return "The invalid datetime.";
        case FAILED_OPEN_FILE:
            return "Failed to open the file.";
        default:
            return "The undefined error.";
    }
}

inline void throwError(ErrorType et, const std::string& extraMsg = "")
{
    std::string errinfo = getErrorMessage(et);
    if (!extraMsg.empty())
        errinfo += ' ' + extraMsg;

    throw std::runtime_error(errinfo);
}

}

// Utility functions.
namespace xlog
{

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

inline double getLocalUtcOffset()
{
    std::time_t localTime;
    std::time(&localTime);

    std::tm gmtTm = *std::gmtime(&localTime);

    std::time_t gmtTime = std::mktime(&gmtTm);

    return static_cast<double>(localTime - gmtTime) / HOUR_SECOND;
}

inline bool isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100!= 0) || year % 400 == 0;
}

inline int yearAllDays(int year)
{
    return isLeapYear(year) ? 366 : 365;
}

inline bool isValidMonth(int month) { return month >= 1 && month <= 12; }

inline bool isLongMonth(int month)
{
    assert(isValidMonth(month));

    return (month <= 7 && month % 2 == 1) || (month >= 8 && month % 2 == 0);
}

inline bool isShortMonth(int month)
{
    assert(isValidMonth(month));

    return (month <= 7 && month % 2 == 0) || (month >= 8 && month % 2 == 1);
}

inline bool isValidDay(int day) { return day >= 1 && day <= 31; }

inline bool isValidDay(int day, int month)
{
    if (!isValidDay(day) || !isValidMonth(month))
        return false;

    if (month == 2 && day > 29)
        return false;

    if (isShortMonth(month) && day > 30)
        return false;

    return true;
}

inline bool isValidDay(int day, int month, int year)
{
    if (!isValidDay(day, month))
        return false;

    if (!isLeapYear(year) && month == 2 && day > 28)
        return false;

    return true;
}

inline bool isValidHour(int hour) { return hour >= 0 && hour <= 23; }

inline bool isValidMinute(int minute) { return minute >= 0 && minute <= 59; }

inline bool isValidSecond(int second) { return second >= 0 && second <= 59; }

inline bool isValidWeekday(int weekday) { return weekday >= 1 && weekday <= 7; }

inline bool isValidYearday(int yearday) { return yearday >= 1 && yearday <= 366; }

inline bool isValidYearday(int yearday, int year)
{
    return (isLeapYear(year) && yearday >= 1 && yearday <= 366) ||
           (yearday >= 1 && yearday <= 365);
}

inline bool isValidUtcOffset(double utcOffset) { return utcOffset >= -12.0 && utcOffset <= 14.0; }

inline std::string getMonthName(int month, LocalFlag localFlag)
{
    if (!isValidMonth(month))
        throwError(INVALID_MONTH);

    switch (localFlag) {
        case EN:
            return MONTH_STR_EN[month - 1];
        case EN_SHORT:
            return MONTH_STR_EN_SHORT[month - 1];
        case CN:
            return MONTH_STR_CN[month - 1];
        case JP:
            return MONTH_STR_JP[month - 1];
        case KR:
            return MONTH_STR_KR[month - 1];
        case NUM:
            return std::to_string(month);
        case NUM_PADDING:
            // Fallthrough.
        default:
            return month < 10 ? '0' + std::to_string(month) : std::to_string(month);
    }
}

inline std::string getWeekName(int week, LocalFlag localFlag)
{
    if(!isValidWeekday(week))
        throwError(INVALID_WEEKDAY);

    switch (localFlag) {
        case EN:
            return WEEK_STR_EN[week - 1];
        case EN_SHORT:
            return WEEK_STR_EN_SHORT[week - 1];
        case CN:
            return WEEK_STR_CN[week - 1];
        case JP:
            return WEEK_STR_JP[week - 1];
        case KR:
            return WEEK_STR_KR[week - 1];
        case NUM:
            // Fallthrough.
        case NUM_PADDING:
            // Fallthrough.
        default:
            return std::to_string(week);
    }
}

inline std::time_t currentTime()
{
    std::time_t t;
    return std::time(&t);
}

inline std::string timeToString(std::time_t time,
                                char timeSeparator = ':', char dateSeparator = '-', char separator = ' ')
{
    std::tm lt = *std::localtime(&time);

    int y = lt.tm_year + 1900;

    std::string hour = lt.tm_hour < 10 ? '0' + std::to_string(lt.tm_hour) : std::to_string(lt.tm_hour);
    std::string min = lt.tm_min < 10 ? '0' + std::to_string(lt.tm_min) : std::to_string(lt.tm_min);
    std::string sec = lt.tm_sec < 10 ? '0' + std::to_string(lt.tm_sec) : std::to_string(lt.tm_sec);
    std::string year = std::to_string(y);
    std::string mon = lt.tm_mon + 1 < 10 ? '0' + std::to_string(lt.tm_mon + 1) : std::to_string(lt.tm_mon + 1);
    std::string day = lt.tm_mday < 10 ? '0' + std::to_string(lt.tm_mday) : std::to_string(lt.tm_mday);

    if (y < 1)
        year = "0000" + year;
    else if (y < 10)
        year = "000" + year;
    else if (y < 100)
        year = "00" + year;
    else if (y < 1000)
        year = '0' + year;

    return year + dateSeparator + mon + dateSeparator + day + separator +
           hour + timeSeparator + min + timeSeparator + sec;
}

inline std::string currentTimeString()
{
    return timeToString(currentTime());
}

inline std::time_t stringToTime(const std::string& str)
{
    std::tm lt;
    lt.tm_year = std::atoi(str.substr(0, 4).c_str()) - 1900;
    lt.tm_mon = std::atoi(str.substr(5, 2).c_str()) - 1;
    lt.tm_mday = std::atoi(str.substr(8, 2).c_str());
    lt.tm_hour = std::atoi(str.substr(11, 2).c_str());
    lt.tm_min = std::atoi(str.substr(14, 2).c_str());
    lt.tm_sec = std::atoi(str.substr(17, 2).c_str());

    return std::mktime(&lt);
}

}

// Classes.
namespace xlog
{

class DateTime
{
public:
    static DateTime fromCurrentTime()
    {
        return DateTime(currentTime());
    }

    static DateTime fromString(const std::string& str)
    {
        return DateTime(stringToTime(str));
    }

    DateTime() = default;

    DateTime(std::time_t time)
    {
        std::tm lt = *std::localtime(&time);

        year_ = lt.tm_year + 1900;
        month_ = lt.tm_mon + 1;
        day_ = lt.tm_mday;
        hour_ = lt.tm_hour;
        min_ = lt.tm_min;
        sec_ = lt.tm_sec;
        weekday_ = lt.tm_wday + 1;
        yearday_ = lt.tm_yday + 1;
    }

    std::string toString(char timeSeparator = ':', char dateSeparator = '-', char separator = ' ') const
    {
        return std::to_string(year_) + dateSeparator + std::to_string(month_) + dateSeparator + std::to_string(day_) +
               separator +
               std::to_string(hour_) + timeSeparator + std::to_string(min_) + timeSeparator + std::to_string(sec_);
    }

    std::string weekName(LocalFlag localFlag) const
    {
        return getWeekName(weekday_, localFlag);
    }

    std::string monthName(LocalFlag localFlag) const
    {
        return getMonthName(month_, localFlag);
    }

    std::time_t time() const
    {
        std::tm lt;
        lt.tm_year = year_ - 1900;
        lt.tm_mon = month_ - 1;
        lt.tm_mday = day_;
        lt.tm_hour = hour_;
        lt.tm_min = min_;
        lt.tm_sec = sec_;

        return std::mktime(&lt);
    }

    int year() const { return year_; }

    unsigned char month() const { return month_; }

    unsigned char day() const { return day_; }

    unsigned char hour() const { return hour_; }

    unsigned char minute() const { return min_; }

    unsigned char second() const { return sec_; }

    unsigned char weekday() const { return weekday_; }

    unsigned char yearday() const { return yearday_; }

private:
    int year_;
    unsigned char month_;
    unsigned char day_;
    unsigned char hour_;
    unsigned char min_;
    unsigned char sec_;
    unsigned char weekday_;
    unsigned char yearday_;
};

struct TimeRange
{
    TimeRange() = default;

    TimeRange(std::time_t start, std::time_t end) :
        start(start), end(end)
    {}

    TimeRange(const std::string& start, const std::string& end) :
        start(stringToTime(start)), end(stringToTime(end))
    {}

    bool isValid() const { return start != -1 && end != -1 && start <= end; }

    bool contains(std::time_t time) const { return start <= time && time <= end; }

    bool contains(const std::string& str) const { return contains(stringToTime(str)); }

    std::time_t start = -1;
    std::time_t end = -1;
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
            throwError(FAILED_OPEN_FILE, filename);
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

    void resetStreamAttributes() { setStreamAttributes(ALL_LEVEL, ALL_TIME, true, true); }

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

        std::time_t time = currentTime();
        logs_.push_back(LogData(level, time, message));

        bool levelpass = (levelFilter_ & level) != 0;
        bool timepass = !timeFilter_.isValid() || timeFilter_.contains(time);
        if (outStream_ && levelpass && timepass)
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
            bool levelpass = (levelFilter & data.level) != 0;
            bool timepass = !timeFilter.isValid() || timeFilter.contains(data.time);
            if (levelpass && timepass)
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
            throwError(FAILED_OPEN_FILE, filename);
        }
    }

private:
    struct LogData
    {
        LogData(Level level, std::time_t time, const std::string& message) :
            level(level), time(time), message(message) 
        {}

        Level level = INFO;
        std::time_t time = -1;
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
            rslt = levelToString(data.level) + ' ' + rslt;

        if (hasTimestamp)
            rslt = timeToString(data.time) + ' ' + rslt;

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

}

// Faster way.
namespace xlog
{

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

#endif // !XLOG_HPP
