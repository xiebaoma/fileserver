/**
 * xiebaoma
 * 2025-06-06
 */

#ifndef __ASYNC_LOG_H__
#define __ASYNC_LOG_H__

#include <stdio.h>
#include <string>
#include <list>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

// #ifdef LOG_EXPORTS
// #define LOG_API __declspec(dllexport)
// #else
// #define LOG_API __declspec(dllimport)
// #endif

#define LOG_API

enum LOG_LEVEL
{
    LOG_LEVEL_TRACE,    // Detailed trace messages
    LOG_LEVEL_DEBUG,    // Debugging information
    LOG_LEVEL_INFO,     // General informational messages
    LOG_LEVEL_WARNING,  // Warnings about potential issues
    LOG_LEVEL_ERROR,    // Business-level errors
    LOG_LEVEL_SYSERROR, // System-level (framework) errors
    LOG_LEVEL_FATAL,    // Fatal errors that terminate the program
    LOG_LEVEL_CRITICAL  // Critical logs that are always printed, regardless of log level
};

/**
 * Logging macro definitions.
 * Automatically include source file and line number for better debugging context.
 */
#define LOGT(...) CAsyncLog::output(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOGD(...) CAsyncLog::output(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOGI(...) CAsyncLog::output(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(...) CAsyncLog::output(LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(...) CAsyncLog::output(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOGSYSE(...) CAsyncLog::output(LOG_LEVEL_SYSERROR, __FILE__, __LINE__, __VA_ARGS__)

/**
 * FATAL logs are written synchronously and immediately cause the program to terminate.
 */
#define LOGF(...) CAsyncLog::output(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/**
 * CRITICAL logs are always written, regardless of the configured log level.
 */
#define LOGC(...) CAsyncLog::output(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __VA_ARGS__)

/**
 * Outputs a binary buffer (e.g., for packet dumps) in hexadecimal format.
 */
#define LOG_DEBUG_BIN(buf, buflength) CAsyncLog::outputBinary(buf, buflength)

class LOG_API CAsyncLog
{
public:
    static bool init(const char *pszLogFileName = nullptr, bool bTruncateLongLine = false, int64_t nRollSize = 10 * 1024 * 1024);
    static void uninit();

    static void setLevel(LOG_LEVEL nLevel);
    static bool isRunning();

    // 不输出线程ID号和所在函数签名、行号
    static bool output(long nLevel, const char *pszFmt, ...);
    // 输出线程ID号和所在函数签名、行号
    static bool output(long nLevel, const char *pszFileName, int nLineNo, const char *pszFmt, ...);

    static bool outputBinary(unsigned char *buffer, size_t size);

private:
    CAsyncLog() = delete;
    ~CAsyncLog() = delete;

    CAsyncLog(const CAsyncLog &rhs) = delete;
    CAsyncLog &operator=(const CAsyncLog &rhs) = delete;

    static void makeLinePrefix(long nLevel, std::string &strPrefix);
    static void getTime(char *pszTime, int nTimeStrLength);
    static bool createNewFile(const char *pszLogFileName);
    static bool writeToFile(const std::string &data);
    // 让程序主动崩溃
    static void crash();

    static const char *ullto4Str(int n);
    static char *formLog(int &index, char *szbuf, size_t size_buf, unsigned char *buffer, size_t size);

    static void writeThreadProc();

private:
    static bool m_bToFile; // 日志写入文件还是写到控制台
    static FILE *m_hLogFile;
    static std::string m_strFileName;                 // 日志文件名
    static std::string m_strFileNamePID;              // 文件名中的进程id
    static bool m_bTruncateLongLog;                   // 长日志是否截断
    static LOG_LEVEL m_nCurrentLevel;                 // 当前日志级别
    static int64_t m_nFileRollSize;                   // 单个日志文件的最大字节数
    static int64_t m_nCurrentWrittenSize;             // 已经写入的字节数目
    static std::list<std::string> m_listLinesToWrite; // 待写入的日志
    static std::unique_ptr<std::thread> m_spWriteThread;
    static std::mutex m_mutexWrite;
    static std::condition_variable m_cvWrite;
    static bool m_bExit;    // 退出标志
    static bool m_bRunning; // 运行标志
};

#endif // !__ASYNC_LOG_H__