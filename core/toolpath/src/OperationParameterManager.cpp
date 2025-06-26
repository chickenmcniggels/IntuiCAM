#include <IntuiCAM/Toolpath/OperationParameterManager.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace IntuiCAM {
namespace Toolpath {

// Helper method implementations for OperationConfig
double OperationParameterManager::OperationConfig::getNumeric(
    const std::string& key, double defaultValue) const {
    auto it = numericParams.find(key);
    return (it != numericParams.end()) ? it->second : defaultValue;
}

std::string OperationParameterManager::OperationConfig::getString(
    const std::string& key, const std::string& defaultValue) const {
    auto it = stringParams.find(key);
    return (it != stringParams.end()) ? it->second : defaultValue;
}

bool OperationParameterManager::OperationConfig::getBoolean(
    const std::string& key, bool defaultValue) const {
    auto it = booleanParams.find(key);
    return (it != booleanParams.end()) ? it->second : defaultValue;
}

void OperationParameterManager::OperationConfig::setNumeric(
    const std::string& key, double value) {
    numericParams[key] = value;
}

void OperationParameterManager::OperationConfig::setString(
    const std::string& key, const std::string& value) {
    stringParams[key] = value;
}

void OperationParameterManager::OperationConfig::setBoolean(
    const std::string& key, bool value) {
    booleanParams[key] = value;
}

// Static material database
const std::map<std::string, OperationParameterManager::MaterialProperties> 
OperationParameterManager::s_materialDatabase = {
    {"steel", {
        "steel", 200.0, 400.0, 50.0, 0.7, 
        0.1, 1000, 1.0, 
        false, false, 1.0
    }},
    {"aluminum", {
        "aluminum", 100.0, 250.0, 200.0, 0.9,
        0.15, 2000, 1.5,
        false, false, 1.2
    }},
    {"brass", {
        "brass", 150.0, 300.0, 120.0, 0.8,
        0.12, 1500, 1.2,
        false, false, 1.1
    }},
    {"stainless_steel", {
        "stainless_steel", 250.0, 600.0, 15.0, 0.5,
        0.08, 800, 0.8,
        true, true, 0.8
    }}
};

OperationParameterManager::ValidationResult 
OperationParameterManager::validateOperationParameters(
    const std::string& operationType,
    const OperationConfig& config,
    const MaterialProperties& material,
    std::shared_ptr<Tool> tool) {
    
    ValidationResult result;
    result.status = ParameterStatus::Complete;
    
    // Get parameter definitions for this operation
    auto paramDefs = getParameterDefinitions(operationType);
    
    // Check all required parameters
    for (const auto& def : paramDefs) {
        if (def.required) {
            if (!validateParameter(def, config, result)) {
                result.status = ParameterStatus::MissingRequired;
            }
        }
    }
    
    // Check parameter conflicts
    checkParameterConflicts(config, result);
    
    // Generate recommendations
    generateRecommendations(operationType, material, tool, result);
    
    // Validate safety constraints
    auto safetyResult = validateSafety(config, material, tool);
    if (!safetyResult.isValid()) {
        result.safetyIssues.insert(result.safetyIssues.end(),
            safetyResult.safetyIssues.begin(), safetyResult.safetyIssues.end());
        result.status = ParameterStatus::InvalidConfiguration;
    }
    
    return result;
}

OperationParameterManager::OperationConfig 
OperationParameterManager::fillMissingParameters(
    const std::string& operationType,
    const OperationConfig& config,
    const MaterialProperties& material,
    std::shared_ptr<Tool> tool) {
    
    OperationConfig filledConfig = config;
    auto paramDefs = getParameterDefinitions(operationType);
    
    // Fill missing numeric parameters
    for (const auto& def : paramDefs) {
        if (filledConfig.numericParams.find(def.name) == filledConfig.numericParams.end()) {
            // Calculate appropriate default based on material and operation
            double defaultValue = def.defaultValue;
            
            // Apply material-specific adjustments
            if (def.name == "feedRate") {
                defaultValue = material.recommendedFeedRate;
            } else if (def.name == "spindleSpeed") {
                defaultValue = material.recommendedSpindleSpeed;
            } else if (def.name == "depthOfCut") {
                defaultValue = material.recommendedDepthOfCut;
            }
            
            filledConfig.setNumeric(def.name, defaultValue);
        }
    }
    
    // Set default string parameters
    if (filledConfig.getString("coolant") == "") {
        filledConfig.setString("coolant", material.requiresCoolant ? "flood" : "none");
    }
    
    // Set default boolean parameters
    if (filledConfig.booleanParams.find("enabled") == filledConfig.booleanParams.end()) {
        filledConfig.setBoolean("enabled", true);
    }
    
    return filledConfig;
}

std::vector<std::string> OperationParameterManager::getRequiredParameters(
    const std::string& operationType) {
    
    std::vector<std::string> required;
    auto paramDefs = getParameterDefinitions(operationType);
    
    for (const auto& def : paramDefs) {
        if (def.required) {
            required.push_back(def.name);
        }
    }
    
    return required;
}

std::map<std::string, std::string> OperationParameterManager::getOptionalParameters(
    const std::string& operationType) {
    
    std::map<std::string, std::string> optional;
    auto paramDefs = getParameterDefinitions(operationType);
    
    for (const auto& def : paramDefs) {
        if (!def.required) {
            optional[def.name] = def.description;
        }
    }
    
    return optional;
}

std::tuple<double, double, double> OperationParameterManager::getParameterConstraints(
    const std::string& operationType,
    const std::string& parameterName) {
    
    auto paramDefs = getParameterDefinitions(operationType);
    
    for (const auto& def : paramDefs) {
        if (def.name == parameterName) {
            return std::make_tuple(def.minValue, def.maxValue, def.defaultValue);
        }
    }
    
    // Default constraints if parameter not found
    return std::make_tuple(0.0, 1000.0, 1.0);
}

OperationParameterManager::MaterialProperties 
OperationParameterManager::getMaterialProperties(const std::string& materialType) {
    
    auto it = s_materialDatabase.find(materialType);
    if (it != s_materialDatabase.end()) {
        return it->second;
    }
    
    // Return default steel properties if material not found
    return s_materialDatabase.at("steel");
}

OperationParameterManager::OperationConfig 
OperationParameterManager::createDefaultConfiguration(
    const std::string& operationType,
    const MaterialProperties& material,
    std::shared_ptr<Tool> tool) {
    
    OperationConfig config;
    config.operationType = operationType;
    config.enabled = true;
    
    // Use fillMissingParameters to populate with defaults
    return fillMissingParameters(operationType, config, material, tool);
}

OperationParameterManager::OperationConfig 
OperationParameterManager::calculateOptimalParameters(
    const std::string& operationType,
    const MaterialProperties& material,
    std::shared_ptr<Tool> tool,
    double partDiameter) {
    
    OperationConfig config = createDefaultConfiguration(operationType, material, tool);
    
    // Calculate optimal cutting speed
    double optimalCuttingSpeed = 200.0 * material.machinabilityRating; // m/min
    double optimalSpindleSpeed = (optimalCuttingSpeed * 1000.0) / (M_PI * partDiameter);
    
    // Clamp to reasonable limits
    optimalSpindleSpeed = std::max(100.0, std::min(3000.0, optimalSpindleSpeed));
    
    config.setNumeric("spindleSpeed", optimalSpindleSpeed);
    
    // Adjust feed rate based on material
    double optimalFeedRate = material.recommendedFeedRate * material.machinabilityRating;
    config.setNumeric("feedRate", optimalFeedRate);
    
    // Adjust depth of cut based on operation and material
    double optimalDepthOfCut = material.recommendedDepthOfCut;
    if (operationType == "Roughing") {
        optimalDepthOfCut *= 1.5; // Deeper cuts for roughing
    } else if (operationType == "Finishing") {
        optimalDepthOfCut *= 0.3; // Shallow cuts for finishing
    }
    
    config.setNumeric("depthOfCut", optimalDepthOfCut);
    
    return config;
}

OperationParameterManager::ValidationResult 
OperationParameterManager::validateSafety(
    const OperationConfig& config,
    const MaterialProperties& material,
    std::shared_ptr<Tool> tool) {
    
    ValidationResult result;
    result.status = ParameterStatus::Complete;
    
    // Check cutting speed limits
    double spindleSpeed = config.getNumeric("spindleSpeed", 0.0);
    if (spindleSpeed > 3000.0) {
        result.safetyIssues.push_back("Spindle speed exceeds safe limits (3000 RPM)");
        result.status = ParameterStatus::InvalidConfiguration;
    }
    
    // Check feed rate limits
    double feedRate = config.getNumeric("feedRate", 0.0);
    if (feedRate > 1.0) {
        result.safetyIssues.push_back("Feed rate exceeds safe limits (1.0 mm/rev)");
        result.status = ParameterStatus::InvalidConfiguration;
    }
    
    // Check material removal rate
    double depthOfCut = config.getNumeric("depthOfCut", 0.0);
    double materialRemovalRate = calculateMaterialRemovalRate(feedRate, depthOfCut, 
        calculateCuttingSpeed(spindleSpeed, 50.0)); // Assume 50mm diameter
    
    if (materialRemovalRate > 1000.0) { // cm³/min
        result.safetyIssues.push_back("Material removal rate too high - risk of tool breakage");
        result.status = ParameterStatus::InvalidConfiguration;
    }
    
    return result;
}

std::vector<OperationParameterManager::ParameterDefinition> 
OperationParameterManager::getParameterDefinitions(const std::string& operationType) {
    
    std::vector<ParameterDefinition> params;
    
    // Common parameters for all operations
    params.push_back({"feedRate", "Feed rate in mm/rev", true, 0.01, 1.0, 0.1, "mm/rev", "cutting"});
    params.push_back({"spindleSpeed", "Spindle speed in RPM", true, 100.0, 3000.0, 1000.0, "RPM", "cutting"});
    params.push_back({"depthOfCut", "Depth of cut in mm", true, 0.1, 10.0, 1.0, "mm", "cutting"});
    
    // Operation-specific parameters
    if (operationType == "Contouring") {
        params.push_back({"finishingPasses", "Number of finishing passes", false, 1.0, 5.0, 2.0, "", "quality"});
        params.push_back({"stockAllowance", "Stock allowance for finishing", false, 0.0, 2.0, 0.2, "mm", "quality"});
    } else if (operationType == "Threading") {
        params.push_back({"threadPitch", "Thread pitch", true, 0.5, 5.0, 1.5, "mm", "cutting"});
        params.push_back({"threadDepth", "Thread depth", true, 0.1, 2.0, 0.8, "mm", "cutting"});
        params.push_back({"threadPasses", "Number of threading passes", false, 1.0, 10.0, 3.0, "", "quality"});
    } else if (operationType == "Parting") {
        params.push_back({"partingWidth", "Parting tool width", true, 1.0, 6.0, 3.0, "mm", "cutting"});
        params.push_back({"peckDepth", "Pecking depth", false, 0.1, 2.0, 0.5, "mm", "cutting"});
    }
    
    return params;
}

bool OperationParameterManager::validateParameter(
    const ParameterDefinition& definition,
    const OperationConfig& config,
    ValidationResult& result) {
    
    double value = config.getNumeric(definition.name, std::numeric_limits<double>::quiet_NaN());
    
    if (std::isnan(value)) {
        result.missingParameters.push_back(definition.name);
        return false;
    }
    
    if (value < definition.minValue || value > definition.maxValue) {
        result.invalidParameters.push_back(
            definition.name + " value " + std::to_string(value) + 
            " is outside valid range [" + std::to_string(definition.minValue) + 
            ", " + std::to_string(definition.maxValue) + "]");
        return false;
    }
    
    return true;
}

double OperationParameterManager::calculateCuttingSpeed(double spindleSpeed, double diameter) {
    return (M_PI * diameter * spindleSpeed) / 1000.0; // m/min
}

double OperationParameterManager::calculateMaterialRemovalRate(
    double feedRate, double depthOfCut, double cuttingSpeed) {
    return feedRate * depthOfCut * cuttingSpeed * 1000.0; // mm³/min
}

void OperationParameterManager::checkParameterConflicts(
    const OperationConfig& config,
    ValidationResult& result) {
    
    // Check for conflicting parameter combinations
    double feedRate = config.getNumeric("feedRate", 0.0);
    double spindleSpeed = config.getNumeric("spindleSpeed", 0.0);
    
    if (feedRate > 0.5 && spindleSpeed > 2000.0) {
        result.warnings.push_back("High feed rate and spindle speed combination may cause poor surface finish");
    }
    
    // Check coolant requirements
    std::string coolant = config.getString("coolant", "none");
    if (spindleSpeed > 1500.0 && coolant == "none") {
        result.warnings.push_back("High spindle speeds typically require coolant for tool life");
    }
}

void OperationParameterManager::generateRecommendations(
    const std::string& operationType,
    const MaterialProperties& material,
    std::shared_ptr<Tool> tool,
    ValidationResult& result) {
    
    if (material.requiresCoolant) {
        result.recommendations.push_back("Use flood coolant for " + material.materialType);
    }
    
    if (material.isWorkHardening) {
        result.recommendations.push_back("Use consistent feed rate to avoid work hardening");
    }
    
    if (operationType == "Finishing") {
        result.recommendations.push_back("Consider lower feed rates for better surface finish");
    }
}

} // namespace Toolpath
} // namespace IntuiCAM 