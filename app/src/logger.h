// =============================================================================
// logger.h -- Simple file logging system for cTetris
// =============================================================================
// Usage:
//   Logger& logger = Logger::getInstance();
//   logger.log("Message: %s", "hello");
//   logger.logAudio("BGM audio stream playing");
//
// Output: game.log in the same directory as executable
// =============================================================================

#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <mutex>

class Logger {
public:
    // Singleton pattern
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Delete copy and move
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * General purpose logging
     * @param fmt Printf-style format string
     */
    void log(const char* fmt, ...) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        va_list args;
        va_start(args, fmt);
        logv(fmt, args);
        va_end(args);
    }

    /**
     * Audio/BGM stream logging
     * @param fmt Printf-style format string
     */
    void logAudio(const char* fmt, ...) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        file_ << "[AUDIO] ";
        va_list args;
        va_start(args, fmt);
        logv(fmt, args);
        va_end(args);
    }

    /**
     * Game event logging (Console, Core, Story)
     * @param module "CONSOLE", "CORE", "STORY", etc.
     * @param fmt Printf-style format string
     */
    void logEvent(const char* module, const char* fmt, ...) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        file_ << "[" << module << "] ";
        va_list args;
        va_start(args, fmt);
        logv(fmt, args);
        va_end(args);
    }

    /**
     * Error logging
     * @param fmt Printf-style format string
     */
    void logError(const char* fmt, ...) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        file_ << "[ERROR] ";
        va_list args;
        va_start(args, fmt);
        logv(fmt, args);
        va_end(args);
    }

    // Flush to disk
    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }

    // Close log file
    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
    }

private:
    Logger() {
        // Open log file for writing (truncate if exists)
        file_.open("game.log", std::ios::out | std::ios::trunc);
        if (file_.is_open()) {
            file_ << "=== cTetris Game Log ===" << std::endl;
            file_ << "Started at: " << getTimestamp() << std::endl;
            file_ << std::endl;
        }
    }

    ~Logger() {
        close();
    }

    void logv(const char* fmt, va_list args) {
        // Write timestamp
        file_ << getTimestamp() << " ";
        
        // Format and write message
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        file_ << buffer << std::endl;
        file_.flush();
    }

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&time_t_now));
        
        std::string result(buf);
        char ms_buf[16];
        snprintf(ms_buf, sizeof(ms_buf), ".%03lld", (long long)ms.count());
        result += ms_buf;
        return result;
    }

    std::ofstream file_;
    std::mutex mutex_;
};
