#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace IntuiCAM {
namespace Toolpath {

// Forward declarations
class Tool;

/**
 * @brief Manages operation parameters, validation, and automatic defaults
 * 
 * This class provides comprehensive parameter management for toolpath operations:
 * - Validates operation configurations for completeness
 * - Automatically fills missing parameters with intelligent defaults
 * - Provides material-specific and tool-specific recommendations
 * - Ensures parameter consistency and safety limits
 * - Supports advanced configuration mode requirements
 */
class OperationParameterManager {
public:
    /**
     * @brief Parameter validation status
     */
    enum class ParameterStatus {
        Complete,               // All required parameters present and valid
        MissingRequired,        // Required parameters missing
        NeedsValidation,        // Parameters present but need validation
        InvalidConfiguration,   // Invalid parameter combinations
        HasWarnings            // Valid but with warnings/recommendations
    };

    /**
     * @brief Material properties for parameter calculation
     */
    struct MaterialProperties {
        std::string materialType = "steel";     // Material classification
        double hardness = 200.0;               // HB (Brinell hardness)
        double tensileStrength = 400.0;        // MPa
        double thermalConductivity = 50.0;     // W/m·K
        double machinabilityRating = 0.7;      // 0.0 to 1.0 (1.0 = free cutting)
        
        // Cutting recommendations
        double recommendedFeedRate = 0.1;      // mm/rev
        double recommendedSpindleSpeed = 1000; // RPM
        double recommendedDepthOfCut = 1.0;    // mm
        
        // Advanced properties
        bool requiresCoolant = false;
        bool isWorkHardening = false;
        double chipEvacuationFactor = 1.0;     // Affects feed rates
    };

    /**
     * @brief Operation configuration structure
     */
    struct OperationConfig {
        std::string operationType;              // "Contouring", "Threading", etc.
        bool enabled = false;
        std::map<std::string, double> numericParams;
        std::map<std::string, std::string> stringParams;
        std::map<std::string, bool> booleanParams;
        
        // Helper methods for type-safe parameter access
        double getNumeric(const std::string& key, double defaultValue = 0.0) const;
        std::string getString(const std::string& key, const std::string& defaultValue = "") const;
        bool getBoolean(const std::string& key, bool defaultValue = false) const;
        
        void setNumeric(const std::string& key, double value);
        void setString(const std::string& key, const std::string& value);
        void setBoolean(const std::string& key, bool value);
    };

    /**
     * @brief Parameter validation result with detailed feedback
     */
    struct ValidationResult {
        ParameterStatus status;
        std::vector<std::string> missingParameters;
        std::vector<std::string> invalidParameters;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        std::vector<std::string> safetyIssues;
        
        // Quality assessment
        double confidenceScore = 1.0;          // 0.0 to 1.0
        bool requiresUserConfirmation = false;
        
        // Default constructor
        ValidationResult() : status(ParameterStatus::Complete) {}
        
        // Helper methods
        bool isValid() const { return status == ParameterStatus::Complete || 
                                     status == ParameterStatus::HasWarnings; }
        bool hasIssues() const { return !missingParameters.empty() || 
                                       !invalidParameters.empty() || 
                                       !safetyIssues.empty(); }
    };

    /**
     * @brief Validate operation parameters for completeness and safety
     * @param operationType Type of operation to validate
     * @param config Current operation configuration
     * @param material Material properties for validation
     * @param tool Tool being used (optional)
     * @return Detailed validation result
     */
    static ValidationResult validateOperationParameters(
        const std::string& operationType,
        const OperationConfig& config,
        const MaterialProperties& material,
        std::shared_ptr<Tool> tool = nullptr);

    /**
     * @brief Automatically fill missing parameters with intelligent defaults
     * @param operationType Type of operation
     * @param config Current configuration (will be modified)
     * @param material Material properties for defaults
     * @param tool Tool being used (optional)
     * @return Updated configuration with filled parameters
     */
    static OperationConfig fillMissingParameters(
        const std::string& operationType,
        const OperationConfig& config,
        const MaterialProperties& material,
        std::shared_ptr<Tool> tool = nullptr);

    /**
     * @brief Get required parameters for an operation type
     * @param operationType Type of operation
     * @return List of required parameter names
     */
    static std::vector<std::string> getRequiredParameters(const std::string& operationType);

    /**
     * @brief Get optional parameters for an operation type
     * @param operationType Type of operation
     * @return List of optional parameter names with descriptions
     */
    static std::map<std::string, std::string> getOptionalParameters(const std::string& operationType);

    /**
     * @brief Get parameter constraints (min/max values)
     * @param operationType Type of operation
     * @param parameterName Name of parameter
     * @return {min_value, max_value, recommended_value}
     */
    static std::tuple<double, double, double> getParameterConstraints(
        const std::string& operationType,
        const std::string& parameterName);

    /**
     * @brief Get material-specific recommendations
     * @param materialType Material classification
     * @return Material properties with recommendations
     */
    static MaterialProperties getMaterialProperties(const std::string& materialType);

    /**
     * @brief Create default configuration for operation type
     * @param operationType Type of operation
     * @param material Material properties
     * @param tool Tool being used (optional)
     * @return Complete default configuration
     */
    static OperationConfig createDefaultConfiguration(
        const std::string& operationType,
        const MaterialProperties& material,
        std::shared_ptr<Tool> tool = nullptr);

    /**
     * @brief Calculate optimal parameters using machining formulas
     * @param operationType Type of operation
     * @param material Material properties
     * @param tool Tool geometry and properties
     * @param partDiameter Workpiece diameter
     * @return Optimized parameter suggestions
     */
    static OperationConfig calculateOptimalParameters(
        const std::string& operationType,
        const MaterialProperties& material,
        std::shared_ptr<Tool> tool,
        double partDiameter);

    /**
     * @brief Validate parameter combinations for safety
     * @param config Operation configuration
     * @param material Material properties
     * @param tool Tool properties
     * @return Safety validation result
     */
    static ValidationResult validateSafety(
        const OperationConfig& config,
        const MaterialProperties& material,
        std::shared_ptr<Tool> tool);

private:
    /**
     * @brief Parameter definitions for each operation type
     */
    struct ParameterDefinition {
        std::string name;
        std::string description;
        bool required = false;
        double minValue = 0.0;
        double maxValue = 1000.0;
        double defaultValue = 0.0;
        std::string units;
        std::string category; // "cutting", "safety", "quality", etc.
    };

    /**
     * @brief Get parameter definitions for operation type
     */
    static std::vector<ParameterDefinition> getParameterDefinitions(
        const std::string& operationType);

    /**
     * @brief Validate individual parameter
     */
    static bool validateParameter(
        const ParameterDefinition& definition,
        const OperationConfig& config,
        ValidationResult& result);

    /**
     * @brief Calculate cutting speed from spindle speed and diameter
     */
    static double calculateCuttingSpeed(double spindleSpeed, double diameter);

    /**
     * @brief Calculate material removal rate
     */
    static double calculateMaterialRemovalRate(
        double feedRate, double depthOfCut, double cuttingSpeed);

    /**
     * @brief Check for parameter conflicts
     */
    static void checkParameterConflicts(
        const OperationConfig& config,
        ValidationResult& result);

    /**
     * @brief Generate recommendations based on material and tool
     */
    static void generateRecommendations(
        const std::string& operationType,
        const MaterialProperties& material,
        std::shared_ptr<Tool> tool,
        ValidationResult& result);

    // Static parameter databases
    static const std::map<std::string, MaterialProperties> s_materialDatabase;
    static const std::map<std::string, std::vector<ParameterDefinition>> s_parameterDefinitions;
};

} // namespace Toolpath
} // namespace IntuiCAM 