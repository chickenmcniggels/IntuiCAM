#pragma once

#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Facing operation for lathe
class FacingOperation : public Operation {
public:
    struct Parameters {
        double startDiameter = 50.0;    // mm
        double endDiameter = 0.0;       // mm (center)
        double stepover = 0.5;          // mm
        double stockAllowance = 0.2;    // mm
        bool roughingOnly = false;
    };
    
private:
    Parameters params_;
    
public:
    FacingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Roughing operation for material removal
class RoughingOperation : public Operation {
public:
    struct Parameters {
        double startDiameter = 50.0;    // mm
        double endDiameter = 20.0;      // mm
        double startZ = 0.0;            // mm
        double endZ = -50.0;            // mm
        double depthOfCut = 2.0;        // mm per pass
        double stockAllowance = 0.5;    // mm for finishing
    };
    
private:
    Parameters params_;
    
public:
    RoughingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Finishing operation for final surface quality
class FinishingOperation : public Operation {
public:
    struct Parameters {
        double targetDiameter = 20.0;   // mm
        double startZ = 0.0;            // mm
        double endZ = -50.0;            // mm
        double surfaceSpeed = 150.0;    // m/min
        double feedRate = 0.05;         // mm/rev
    };
    
private:
    Parameters params_;
    
public:
    FinishingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Parting operation for cutting off parts
class PartingOperation : public Operation {
public:
    struct Parameters {
        double partingDiameter = 20.0;  // mm
        double partingZ = -50.0;        // mm
        double centerHoleDiameter = 3.0; // mm (0 for solid)
        double feedRate = 0.02;         // mm/rev
        double retractDistance = 2.0;   // mm
    };
    
private:
    Parameters params_;
    
public:
    PartingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Threading operation for creating threads
class ThreadingOperation : public Operation {
public:
    struct Parameters {
        double majorDiameter = 20.0;    // mm
        double pitch = 1.5;             // mm
        double threadLength = 30.0;     // mm
        double startZ = 0.0;            // mm
        int numberOfPasses = 5;
        bool isMetric = true;
    };
    
private:
    Parameters params_;
    
public:
    ThreadingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Grooving operation for creating grooves
class GroovingOperation : public Operation {
public:
    struct Parameters {
        double grooveDiameter = 20.0;   // mm
        double grooveWidth = 3.0;       // mm
        double grooveDepth = 2.0;       // mm
        double grooveZ = -25.0;         // mm
        double feedRate = 0.02;         // mm/rev
    };
    
private:
    Parameters params_;
    
public:
    GroovingOperation(const std::string& name, std::shared_ptr<Tool> tool);
    
    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }
    
    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Chamfering operation for edge breaks
class ChamferingOperation : public Operation {
public:
    struct Parameters {
        double chamferSize = 0.5; // mm
    };

private:
    Parameters params_;

public:
    ChamferingOperation(const std::string& name, std::shared_ptr<Tool> tool);

    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }

    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

// Contouring operation that combines facing, roughing and finishing
class ContouringOperation : public Operation {
public:
    struct Parameters {
        FacingOperation::Parameters facing;
        RoughingOperation::Parameters roughing;
        FinishingOperation::Parameters finishing;
    };

private:
    Parameters params_;

public:
    ContouringOperation(const std::string& name, std::shared_ptr<Tool> tool);

    void setParameters(const Parameters& params) { params_ = params; }
    const Parameters& getParameters() const { return params_; }

    std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) override;
    bool validate() const override;
};

} // namespace Toolpath
} // namespace IntuiCAM
