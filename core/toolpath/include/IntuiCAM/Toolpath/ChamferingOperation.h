#pragma once

#include <IntuiCAM/Toolpath/Types.h>
#include <vector>

namespace IntuiCAM {
namespace Toolpath {

/**
 * @brief Chamfering operation for creating chamfers on edges
 * 
 * Supports various chamfer types including 45-degree chamfers, angled chamfers,
 * and radius chamfers on both internal and external features.
 */
class ChamferingOperation : public Operation {
public:
    enum class ChamferType {
        Linear,         // Linear chamfer at specified angle
        Radius,         // Radius chamfer (rounded corner)
        CustomAngle     // Custom angle chamfer
    };
    
    struct Parameters {
        ChamferType chamferType = ChamferType::Linear;
        double chamferSize = 0.5;       // mm - size of chamfer
        double chamferAngle = 45.0;     // degrees - angle of chamfer
        double feedRate = 100.0;        // mm/min - chamfering feed rate
        double spindleSpeed = 1000.0;   // RPM - spindle speed
        double safetyHeight = 5.0;      // mm - safe height above part
        double startZ = 0.0;            // mm - Z position of chamfer start
        double startDiameter = 20.0;    // mm - diameter at chamfer start
        double endDiameter = 18.0;      // mm - diameter at chamfer end
        bool isExternal = true;         // true for external, false for internal
    };
    
private:
    Parameters params_;
    
public:
    ChamferingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    // Static validation method
    static std::string validateParameters(const Parameters& params);
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
    
private:
    // Helper methods for different chamfer types
    std::unique_ptr<Toolpath> generateLinearChamfer();
    std::unique_ptr<Toolpath> generateRadiusChamfer();
    std::unique_ptr<Toolpath> generateCustomAngleChamfer();
};

} // namespace Toolpath
} // namespace IntuiCAM 