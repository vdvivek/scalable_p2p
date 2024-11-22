#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

#define RED "\033[01;31m"
#define GREEN "\033[01;32m"
#define YELLOW "\033[01;33m"
#define BLUE "\033[01;34m"
#define RESET "\033[00m"

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
public:
  Logger(const Logger &) = delete;            // Delete copy constructor
  Logger &operator=(const Logger &) = delete; // Delete copy assignment operator

  static Logger &getInstance() {
    static Logger instance; // Singleton instance
    return instance;
  }

  void setLogFile(const std::string &filename) {
    std::lock_guard<std::mutex> lock(mutex);
    if (logFile.is_open()) {
      logFile.close();
    }
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
      std::cerr << "[ERROR] Failed to open log file: " << filename << std::endl;
    }
  }

  void log(LogLevel level, const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex);
    std::string logMessage = " [" + logLevelToString(level) + "] " + message;

    if (logFile.is_open()) {
      logFile << logMessage << std::endl;
    }

    switch (level) {
    case LogLevel::INFO: {
      std::cout << GREEN << logMessage << RESET << std::endl;
      break;
    }
    case LogLevel::DEBUG: {
      std::cout << BLUE << logMessage << RESET << std::endl;
      break;
    }

    case LogLevel::ERROR: {
      std::cout << RED << logMessage << RESET << std::endl;
      break;
    }

    case LogLevel::WARNING: {
      std::cout << YELLOW << logMessage << RESET << std::endl;
      break;
    }
    }
  }

  void setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);
    minLogLevel = level;
  }

private:
  Logger() = default; // Private constructor for singleton

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
        {LogLevel::ERROR, "ERROR"}};
    return levelToString.at(level);
  }
};

// Declare a global reference to the logger
extern Logger &logger;

#endif // LOGGER_H
