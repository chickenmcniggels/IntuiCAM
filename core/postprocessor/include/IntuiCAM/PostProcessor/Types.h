#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace PostProcessor {

// G-code generation and machine-specific adaptations

struct MachineConfig {
    std::string machineName = "Generic Lathe";
    std::string units = "mm";           // "mm" or "inch"
    bool absoluteCoordinates = true;    // G90/G91
    bool spindleClockwise = true;       // M3/M4
    double rapidFeedRate = 5000.0;      // mm/min
    double maxSpindleSpeed = 3000.0;    // RPM

    // Machine limits
    double maxX = 200.0;                // mm
    double maxZ = 300.0;                // mm
    double minX = 0.0;                  // mm
    double minZ = -300.0;               // mm

    // Safety settings
    bool useToolLengthCompensation = true;
    bool useCoolant = true;
    double safeRetractZ = 5.0;          // mm
};

class GCodeGenerator {
public:
    
    struct PostProcessorOptions {
        bool includeComments = true;
        bool includeLineNumbers = true;
        bool optimizeRapids = true;
        bool addSafetyMoves = true;
        int lineNumberIncrement = 10;
        std::string programNumber = "1001";
    };
    
private:
    MachineConfig config_;
    PostProcessorOptions options_;
    int currentLineNumber_;
    
public:
    GCodeGenerator(const MachineConfig& config);
    GCodeGenerator();
    
    // Configuration
    void setMachineConfig(const MachineConfig& config) { config_ = config; }
    void setOptions(const PostProcessorOptions& options) { options_ = options; }
    
    // G-code generation
    std::string generateGCode(const std::vector<std::shared_ptr<Toolpath::Toolpath>>& toolpaths);
    std::string generateGCode(const Toolpath::Toolpath& toolpath);
    
    // Individual G-code commands
    std::string generateProgramHeader(const std::string& programName = "");
    std::string generateProgramFooter();
    std::string generateToolChange(const Toolpath::Tool& tool, int toolNumber);
    std::string generateMovement(const Toolpath::Movement& movement);
    std::string generateSpindleControl(double rpm, bool clockwise = true);
    std::string generateCoolantControl(bool on);
    
    // Validation
    bool validateToolpath(const Toolpath::Toolpath& toolpath) const;
    std::vector<std::string> checkMachineLimits(const Toolpath::Toolpath& toolpath) const;
    
private:
    std::string formatLineNumber();
    std::string formatCoordinate(double value, char axis) const;
    std::string formatFeedRate(double feedRate) const;
    std::string formatSpindleSpeed(double rpm) const;
};


// Post-processor for specific machine types
class PostProcessor {
public:
    enum class MachineType {
        GenericLathe,
        Fanuc,
        Haas,
        Mazak,
        Okuma,
        Siemens
    };
    
    struct ProcessingResult {
        std::string gcode;
        bool success = false;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        double estimatedTime = 0.0;        // minutes
    };
    
private:
    MachineType machineType_;
    std::unique_ptr<GCodeGenerator> generator_;
    
public:
    PostProcessor(MachineType type);
    
    // Main processing
    ProcessingResult process(const std::vector<std::shared_ptr<Toolpath::Toolpath>>& toolpaths);
    ProcessingResult process(const Toolpath::Toolpath& toolpath);
    
    // Machine-specific customization
    void customizeForMachine(MachineType type);
    void loadMachineProfile(const std::string& profilePath);
    void saveMachineProfile(const std::string& profilePath) const;
    
    // Factory methods
    static std::unique_ptr<PostProcessor> createForMachine(MachineType type);
    static std::vector<MachineType> getSupportedMachines();
    static std::string getMachineName(MachineType type);
};

// Machine-specific dialects and customizations
namespace Dialects {

class FanucDialect {
public:
    static std::string formatMovement(const Toolpath::Movement& movement);
    static std::string formatToolChange(int toolNumber);
    static std::string formatSpindleControl(double rpm, bool clockwise);
    static std::string getProgramHeader();
    static std::string getProgramFooter();
};

class HaasDialect {
public:
    static std::string formatMovement(const Toolpath::Movement& movement);
    static std::string formatToolChange(int toolNumber);
    static std::string formatSpindleControl(double rpm, bool clockwise);
    static std::string getProgramHeader();
    static std::string getProgramFooter();
};

} // namespace Dialects

} // namespace PostProcessor
} // namespace IntuiCAM 