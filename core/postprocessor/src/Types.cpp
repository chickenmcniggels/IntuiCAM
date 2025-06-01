#include <IntuiCAM/PostProcessor/Types.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace IntuiCAM {
namespace PostProcessor {

// GCodeGenerator implementation
GCodeGenerator::GCodeGenerator(const MachineConfig& config)
    : config_(config), currentLineNumber_(10) {
}

std::string GCodeGenerator::generateGCode(const std::vector<std::shared_ptr<Toolpath::Toolpath>>& toolpaths) {
    std::ostringstream gcode;
    
    // Program header
    gcode << generateProgramHeader();
    
    // Process each toolpath
    for (const auto& toolpath : toolpaths) {
        if (toolpath) {
            gcode << generateGCode(*toolpath);
        }
    }
    
    // Program footer
    gcode << generateProgramFooter();
    
    return gcode.str();
}

std::string GCodeGenerator::generateGCode(const Toolpath::Toolpath& toolpath) {
    std::ostringstream gcode;
    
    // Tool change if needed
    if (toolpath.getTool()) {
        gcode << generateToolChange(*toolpath.getTool(), 1);
    }
    
    // Process movements
    for (const auto& movement : toolpath.getMovements()) {
        gcode << generateMovement(movement);
    }
    
    return gcode.str();
}

std::string GCodeGenerator::generateProgramHeader(const std::string& programName) {
    std::ostringstream header;
    
    if (options_.includeComments) {
        header << "; IntuiCAM Generated G-Code\n";
        header << "; Program: " << (programName.empty() ? options_.programNumber : programName) << "\n";
        header << "; Machine: " << config_.machineName << "\n";
        header << "; Units: " << config_.units << "\n\n";
    }
    
    // Program number
    header << "O" << options_.programNumber << "\n";
    
    // Initialize machine
    header << formatLineNumber() << "G21"; // Metric units
    if (options_.includeComments) header << " ; Metric units";
    header << "\n";
    
    header << formatLineNumber() << "G90"; // Absolute coordinates
    if (options_.includeComments) header << " ; Absolute coordinates";
    header << "\n";
    
    header << formatLineNumber() << "G40"; // Cancel cutter compensation
    if (options_.includeComments) header << " ; Cancel cutter compensation";
    header << "\n";
    
    return header.str();
}

std::string GCodeGenerator::generateProgramFooter() {
    std::ostringstream footer;
    
    // Stop spindle and coolant
    footer << formatLineNumber() << "M5";
    if (options_.includeComments) footer << " ; Stop spindle";
    footer << "\n";
    
    footer << formatLineNumber() << "M9";
    if (options_.includeComments) footer << " ; Coolant off";
    footer << "\n";
    
    // Return to home
    footer << formatLineNumber() << "G28 U0 W0";
    if (options_.includeComments) footer << " ; Return to home";
    footer << "\n";
    
    // End program
    footer << formatLineNumber() << "M30";
    if (options_.includeComments) footer << " ; End program";
    footer << "\n";
    
    return footer.str();
}

std::string GCodeGenerator::generateToolChange(const Toolpath::Tool& tool, int toolNumber) {
    std::ostringstream toolChange;
    
    toolChange << formatLineNumber() << "T" << std::setfill('0') << std::setw(2) << toolNumber;
    if (options_.includeComments) {
        toolChange << " ; Tool change: " << tool.getName();
    }
    toolChange << "\n";
    
    // Set cutting parameters
    const auto& params = tool.getCuttingParameters();
    toolChange << generateSpindleControl(params.spindleSpeed, true);
    
    return toolChange.str();
}

std::string GCodeGenerator::generateMovement(const Toolpath::Movement& movement) {
    std::ostringstream move;
    
    move << formatLineNumber();
    
    switch (movement.type) {
        case Toolpath::MovementType::Rapid:
            move << "G0";
            break;
        case Toolpath::MovementType::Linear:
            move << "G1";
            break;
        case Toolpath::MovementType::CircularCW:
            move << "G2";
            break;
        case Toolpath::MovementType::CircularCCW:
            move << "G3";
            break;
        case Toolpath::MovementType::Dwell:
            move << "G4 P" << std::fixed << std::setprecision(2) << 1.0; // 1 second dwell
            break;
        default:
            break;
    }
    
    // Add coordinates
    if (movement.type != Toolpath::MovementType::Dwell) {
        move << formatCoordinate(movement.position.x, 'X');
        move << formatCoordinate(movement.position.z, 'Z');
        
        if (movement.feedRate > 0.0 && movement.type != Toolpath::MovementType::Rapid) {
            move << formatFeedRate(movement.feedRate);
        }
    }
    
    if (options_.includeComments && !movement.comment.empty()) {
        move << " ; " << movement.comment;
    }
    
    move << "\n";
    return move.str();
}

std::string GCodeGenerator::generateSpindleControl(double rpm, bool clockwise) {
    std::ostringstream spindle;
    
    spindle << formatLineNumber();
    spindle << (clockwise ? "M3" : "M4");
    spindle << formatSpindleSpeed(rpm);
    
    if (options_.includeComments) {
        spindle << " ; Spindle " << (clockwise ? "CW" : "CCW") << " at " << rpm << " RPM";
    }
    spindle << "\n";
    
    return spindle.str();
}

std::string GCodeGenerator::generateCoolantControl(bool on) {
    std::ostringstream coolant;
    
    coolant << formatLineNumber() << (on ? "M8" : "M9");
    if (options_.includeComments) {
        coolant << " ; Coolant " << (on ? "on" : "off");
    }
    coolant << "\n";
    
    return coolant.str();
}

bool GCodeGenerator::validateToolpath(const Toolpath::Toolpath& toolpath) const {
    // Basic validation
    return !toolpath.getMovements().empty();
}

std::vector<std::string> GCodeGenerator::checkMachineLimits(const Toolpath::Toolpath& toolpath) const {
    std::vector<std::string> warnings;
    
    auto bbox = toolpath.getBoundingBox();
    
    if (bbox.max.x > config_.maxX) {
        warnings.push_back("X coordinate exceeds machine limit");
    }
    if (bbox.min.z < config_.minZ) {
        warnings.push_back("Z coordinate exceeds machine limit");
    }
    
    return warnings;
}

std::string GCodeGenerator::formatLineNumber() {
    if (!options_.includeLineNumbers) {
        return "";
    }
    
    std::ostringstream line;
    line << "N" << currentLineNumber_ << " ";
    currentLineNumber_ += options_.lineNumberIncrement;
    return line.str();
}

std::string GCodeGenerator::formatCoordinate(double value, char axis) const {
    std::ostringstream coord;
    coord << " " << axis << std::fixed << std::setprecision(3) << value;
    return coord.str();
}

std::string GCodeGenerator::formatFeedRate(double feedRate) const {
    std::ostringstream feed;
    feed << " F" << std::fixed << std::setprecision(1) << feedRate;
    return feed.str();
}

std::string GCodeGenerator::formatSpindleSpeed(double rpm) const {
    std::ostringstream speed;
    speed << " S" << std::fixed << std::setprecision(0) << rpm;
    return speed.str();
}

// PostProcessor implementation
PostProcessor::PostProcessor(MachineType type) : machineType_(type) {
    customizeForMachine(type);
}

PostProcessor::ProcessingResult PostProcessor::process(const std::vector<std::shared_ptr<Toolpath::Toolpath>>& toolpaths) {
    ProcessingResult result;
    
    try {
        result.gcode = generator_->generateGCode(toolpaths);
        result.success = true;
        
        // Estimate time
        for (const auto& toolpath : toolpaths) {
            if (toolpath) {
                result.estimatedTime += toolpath->estimateMachiningTime();
            }
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.errors.push_back(e.what());
    }
    
    return result;
}

PostProcessor::ProcessingResult PostProcessor::process(const Toolpath::Toolpath& toolpath) {
    ProcessingResult result;
    
    try {
        result.gcode = generator_->generateGCode(toolpath);
        result.success = true;
        result.estimatedTime = toolpath.estimateMachiningTime();
        
        // Check machine limits
        auto warnings = generator_->checkMachineLimits(toolpath);
        result.warnings = warnings;
    } catch (const std::exception& e) {
        result.success = false;
        result.errors.push_back(e.what());
    }
    
    return result;
}

void PostProcessor::customizeForMachine(MachineType type) {
    GCodeGenerator::MachineConfig config;
    
    switch (type) {
        case MachineType::Fanuc:
            config.machineName = "Fanuc Lathe";
            break;
        case MachineType::Haas:
            config.machineName = "Haas Lathe";
            break;
        case MachineType::Mazak:
            config.machineName = "Mazak Lathe";
            break;
        case MachineType::Okuma:
            config.machineName = "Okuma Lathe";
            break;
        case MachineType::Siemens:
            config.machineName = "Siemens Lathe";
            break;
        default:
            config.machineName = "Generic Lathe";
            break;
    }
    
    generator_ = std::make_unique<GCodeGenerator>(config);
}

void PostProcessor::loadMachineProfile(const std::string& profilePath) {
    // Placeholder - would load from file
}

void PostProcessor::saveMachineProfile(const std::string& profilePath) const {
    // Placeholder - would save to file
}

std::unique_ptr<PostProcessor> PostProcessor::createForMachine(MachineType type) {
    return std::make_unique<PostProcessor>(type);
}

std::vector<PostProcessor::MachineType> PostProcessor::getSupportedMachines() {
    return {
        MachineType::GenericLathe,
        MachineType::Fanuc,
        MachineType::Haas,
        MachineType::Mazak,
        MachineType::Okuma,
        MachineType::Siemens
    };
}

std::string PostProcessor::getMachineName(MachineType type) {
    switch (type) {
        case MachineType::Fanuc: return "Fanuc";
        case MachineType::Haas: return "Haas";
        case MachineType::Mazak: return "Mazak";
        case MachineType::Okuma: return "Okuma";
        case MachineType::Siemens: return "Siemens";
        default: return "Generic Lathe";
    }
}

// Dialect implementations
namespace Dialects {

std::string FanucDialect::formatMovement(const Toolpath::Movement& movement) {
    // Fanuc-specific formatting
    return ""; // Placeholder
}

std::string FanucDialect::formatToolChange(int toolNumber) {
    return "T" + std::to_string(toolNumber);
}

std::string FanucDialect::formatSpindleControl(double rpm, bool clockwise) {
    return std::string(clockwise ? "M3" : "M4") + " S" + std::to_string(static_cast<int>(rpm));
}

std::string FanucDialect::getProgramHeader() {
    return "; Fanuc Lathe Program\n";
}

std::string FanucDialect::getProgramFooter() {
    return "M30\n";
}

std::string HaasDialect::formatMovement(const Toolpath::Movement& movement) {
    // Haas-specific formatting
    return ""; // Placeholder
}

std::string HaasDialect::formatToolChange(int toolNumber) {
    return "T" + std::to_string(toolNumber);
}

std::string HaasDialect::formatSpindleControl(double rpm, bool clockwise) {
    return std::string(clockwise ? "M3" : "M4") + " S" + std::to_string(static_cast<int>(rpm));
}

std::string HaasDialect::getProgramHeader() {
    return "; Haas Lathe Program\n";
}

std::string HaasDialect::getProgramFooter() {
    return "M30\n";
}

} // namespace Dialects

} // namespace PostProcessor
} // namespace IntuiCAM 