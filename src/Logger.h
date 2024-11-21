#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <memory>
#include <ctime>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex);
        if (logFile.is_open()) {
            logFile.close();
        }
        logFile.open(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "[ERROR] Failed to open log file: " << filename << std::endl;
        }
    }

    void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);
        std::string logMessage = " [" + logLevelToString(level) + "] " + message;

        // Log to file if enabled
        if (logFile.is_open()) {
            logFile << logMessage << std::endl;
        }

        // Log to console
        std::cout << logMessage << std::endl;
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex);
        minLogLevel = level;
    }

private:
    Logger() : minLogLevel(LogLevel::DEBUG) {}

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    std::ofstream logFile;
    std::mutex mutex;
    LogLevel minLogLevel;

    static std::string logLevelToString(LogLevel level) {
        static const std::unordered_map<LogLevel, std::string> levelToString = {
            {LogLevel::DEBUG, "DEBUG"},
            {LogLevel::INFO, "INFO"},
            {LogLevel::WARNING, "WARNING"},
            {LogLevel::ERROR, "ERROR"}
        };
        return levelToString.at(level);
    }
};

#endif // LOGGER_H
