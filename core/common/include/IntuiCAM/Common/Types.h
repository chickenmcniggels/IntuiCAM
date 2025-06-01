#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <optional>

namespace IntuiCAM {
namespace Common {

// Basic type aliases for consistency
using String = std::string;
using StringVector = std::vector<std::string>;
using StringMap = std::map<std::string, std::string>;

// Smart pointer aliases
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Function type aliases
template<typename T>
using Function = std::function<T>;

// Result type for operations that can fail
template<typename T>
class Result {
private:
    bool success_;
    T value_;
    std::string errorMessage_;
    
public:
    Result(const T& value) : success_(true), value_(value) {}
    Result(const std::string& error) : success_(false), errorMessage_(error) {}
    
    bool isSuccess() const { return success_; }
    bool isError() const { return !success_; }
    
    const T& getValue() const { return value_; }
    const std::string& getError() const { return errorMessage_; }
    
    // Convenience operators
    explicit operator bool() const { return success_; }
    const T& operator*() const { return value_; }
    const T* operator->() const { return &value_; }
};

// Optional type for values that may not exist
template<typename T>
using Optional = std::optional<T>;

// Error handling
class Exception : public std::exception {
private:
    std::string message_;
    
public:
    explicit Exception(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
};

class GeometryException : public Exception {
public:
    explicit GeometryException(const std::string& message) 
        : Exception("Geometry Error: " + message) {}
};

class ToolpathException : public Exception {
public:
    explicit ToolpathException(const std::string& message) 
        : Exception("Toolpath Error: " + message) {}
};

class SimulationException : public Exception {
public:
    explicit SimulationException(const std::string& message) 
        : Exception("Simulation Error: " + message) {}
};

// Logging interface
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    virtual ~Logger() = default;
    virtual void log(LogLevel level, const std::string& message) = 0;
    
    void debug(const std::string& message) { log(LogLevel::Debug, message); }
    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }
    void critical(const std::string& message) { log(LogLevel::Critical, message); }
};

// Global logger instance
Logger* getGlobalLogger();
void setGlobalLogger(std::unique_ptr<Logger> logger);

// Utility macros for logging
#define LOG_DEBUG(msg) if(auto* logger = IntuiCAM::Common::getGlobalLogger()) logger->debug(msg)
#define LOG_INFO(msg) if(auto* logger = IntuiCAM::Common::getGlobalLogger()) logger->info(msg)
#define LOG_WARNING(msg) if(auto* logger = IntuiCAM::Common::getGlobalLogger()) logger->warning(msg)
#define LOG_ERROR(msg) if(auto* logger = IntuiCAM::Common::getGlobalLogger()) logger->error(msg)
#define LOG_CRITICAL(msg) if(auto* logger = IntuiCAM::Common::getGlobalLogger()) logger->critical(msg)

// Progress reporting interface
class ProgressReporter {
public:
    virtual ~ProgressReporter() = default;
    virtual void setProgress(double percentage) = 0;  // 0.0 to 100.0
    virtual void setStatus(const std::string& status) = 0;
    virtual void setSubProgress(double percentage) = 0;
    virtual bool isCancelled() const = 0;
};

// Configuration management
class Configuration {
private:
    std::map<std::string, std::string> values_;
    
public:
    void setValue(const std::string& key, const std::string& value);
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    
    template<typename T>
    void setValue(const std::string& key, const T& value);
    
    template<typename T>
    T getValue(const std::string& key, const T& defaultValue = T{}) const;
    
    bool hasKey(const std::string& key) const;
    void removeKey(const std::string& key);
    
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
};

// Units and conversions
enum class LengthUnit {
    Millimeter,
    Inch,
    Meter
};

enum class AngleUnit {
    Degree,
    Radian
};

class UnitConverter {
public:
    static double convertLength(double value, LengthUnit from, LengthUnit to);
    static double convertAngle(double value, AngleUnit from, AngleUnit to);
    
    static std::string getLengthUnitString(LengthUnit unit);
    static std::string getAngleUnitString(AngleUnit unit);
};

// Math utilities
namespace Math {
    constexpr double PI = 3.14159265358979323846;
    constexpr double EPSILON = 1e-9;
    
    bool isEqual(double a, double b, double tolerance = EPSILON);
    bool isZero(double value, double tolerance = EPSILON);
    double clamp(double value, double min, double max);
    double lerp(double a, double b, double t);
    double degToRad(double degrees);
    double radToDeg(double radians);
}

} // namespace Common
} // namespace IntuiCAM 