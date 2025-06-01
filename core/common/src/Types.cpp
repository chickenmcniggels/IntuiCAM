#include <IntuiCAM/Common/Types.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace IntuiCAM {
namespace Common {

// Global logger instance
static std::unique_ptr<Logger> g_logger = nullptr;

Logger* getGlobalLogger() {
    return g_logger.get();
}

void setGlobalLogger(std::unique_ptr<Logger> logger) {
    g_logger = std::move(logger);
}

// Configuration implementation
void Configuration::setValue(const std::string& key, const std::string& value) {
    values_[key] = value;
}

std::string Configuration::getValue(const std::string& key, const std::string& defaultValue) const {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : defaultValue;
}

bool Configuration::hasKey(const std::string& key) const {
    return values_.find(key) != values_.end();
}

void Configuration::removeKey(const std::string& key) {
    values_.erase(key);
}

bool Configuration::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            setValue(key, value);
        }
    }
    
    return true;
}

bool Configuration::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# IntuiCAM Configuration File\n";
    for (const auto& pair : values_) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    return true;
}

// UnitConverter implementation
double UnitConverter::convertLength(double value, LengthUnit from, LengthUnit to) {
    if (from == to) {
        return value;
    }
    
    // Convert to millimeters first
    double mm_value = value;
    switch (from) {
        case LengthUnit::Inch:
            mm_value = value * 25.4;
            break;
        case LengthUnit::Meter:
            mm_value = value * 1000.0;
            break;
        case LengthUnit::Millimeter:
            // Already in mm
            break;
    }
    
    // Convert from millimeters to target unit
    switch (to) {
        case LengthUnit::Inch:
            return mm_value / 25.4;
        case LengthUnit::Meter:
            return mm_value / 1000.0;
        case LengthUnit::Millimeter:
            return mm_value;
    }
    
    return value; // Fallback
}

double UnitConverter::convertAngle(double value, AngleUnit from, AngleUnit to) {
    if (from == to) {
        return value;
    }
    
    if (from == AngleUnit::Degree && to == AngleUnit::Radian) {
        return value * Math::PI / 180.0;
    } else if (from == AngleUnit::Radian && to == AngleUnit::Degree) {
        return value * 180.0 / Math::PI;
    }
    
    return value; // Fallback
}

std::string UnitConverter::getLengthUnitString(LengthUnit unit) {
    switch (unit) {
        case LengthUnit::Millimeter: return "mm";
        case LengthUnit::Inch: return "in";
        case LengthUnit::Meter: return "m";
    }
    return "mm"; // Default
}

std::string UnitConverter::getAngleUnitString(AngleUnit unit) {
    switch (unit) {
        case AngleUnit::Degree: return "deg";
        case AngleUnit::Radian: return "rad";
    }
    return "deg"; // Default
}

// Math utilities implementation
namespace Math {

bool isEqual(double a, double b, double tolerance) {
    return std::abs(a - b) <= tolerance;
}

bool isZero(double value, double tolerance) {
    return std::abs(value) <= tolerance;
}

double clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

double degToRad(double degrees) {
    return degrees * PI / 180.0;
}

double radToDeg(double radians) {
    return radians * 180.0 / PI;
}

} // namespace Math

} // namespace Common
} // namespace IntuiCAM 