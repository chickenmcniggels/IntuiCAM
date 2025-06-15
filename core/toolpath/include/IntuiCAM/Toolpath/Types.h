#pragma once

#include <memory>
#include <vector>
#include <string>
#include <IntuiCAM/Geometry/Types.h>
#include <IntuiCAM/Common/Types.h>

namespace IntuiCAM {
namespace Toolpath {

// Forward declarations
class Tool;
class Operation;
class Toolpath;

// Tool definition with geometry and cutting parameters
class Tool {
public:
    enum class Type {
        Turning,
        Facing,
        Parting,
        Threading,
        Grooving,
        Chamfering,
        Contouring
    };
    
    struct CuttingParameters {
        double feedRate = 0.1;          // mm/rev
        double spindleSpeed = 1000;     // RPM
        double depthOfCut = 1.0;        // mm
        double stepover = 0.5;          // mm
    };
    
    struct Geometry {
        double tipRadius = 0.4;         // mm
        double clearanceAngle = 7.0;    // degrees
        double rakeAngle = 0.0;         // degrees
        double insertWidth = 3.0;       // mm
    };
    
private:
    Type type_;
    std::string name_;
    CuttingParameters cuttingParams_;
    Geometry geometry_;
    
public:
    Tool(Type type, const std::string& name);
    
    // Getters/Setters
    Type getType() const { return type_; }
    const std::string& getName() const { return name_; }
    const CuttingParameters& getCuttingParameters() const { return cuttingParams_; }
    const Geometry& getGeometry() const { return geometry_; }
    
    void setCuttingParameters(const CuttingParameters& params) { cuttingParams_ = params; }
    void setGeometry(const Geometry& geom) { geometry_ = geom; }
};

// Movement types for toolpath generation
enum class MovementType {
    Rapid,          // G0 - rapid positioning
    Linear,         // G1 - linear interpolation
    CircularCW,     // G2 - circular interpolation clockwise
    CircularCCW,    // G3 - circular interpolation counter-clockwise
    Dwell,          // G4 - dwell/pause
    ToolChange      // Tool change operation
};

// Individual toolpath movement
struct Movement {
    MovementType type;
    Geometry::Point3D position;
    double feedRate = 0.0;
    double spindleSpeed = 0.0;
    std::string comment;
    
    Movement(MovementType t, const Geometry::Point3D& pos) 
        : type(t), position(pos) {}
};

// Sequence of movements with types and parameters
class Toolpath {
private:
    std::vector<Movement> movements_;
    std::shared_ptr<Tool> tool_;
    std::string name_;
    
public:
    Toolpath(const std::string& name, std::shared_ptr<Tool> tool);
    
    // Movement operations
    void addMovement(const Movement& movement);
    void addRapidMove(const Geometry::Point3D& position);
    void addLinearMove(const Geometry::Point3D& position, double feedRate);
    void addCircularMove(const Geometry::Point3D& position, const Geometry::Point3D& center,
                        bool clockwise, double feedRate);
    void addDwell(double seconds);
    void appendToolpath(const Toolpath& other);
    
    // Getters
    const std::vector<Movement>& getMovements() const { return movements_; }
    std::shared_ptr<Tool> getTool() const { return tool_; }
    const std::string& getName() const { return name_; }
    
    // Analysis
    size_t getMovementCount() const { return movements_.size(); }
    double estimateMachiningTime() const;
    Geometry::BoundingBox getBoundingBox() const;
    
    // Optimization
    void optimizeToolpath();
    void removeRedundantMoves();
};

// Base class for machining operations
class Operation {
public:
    enum class Type {
        Facing,
        Roughing,
        Finishing,
        Parting,
        Threading,
        Grooving,
        Chamfering,
        Contouring
    };
    
protected:
    Type type_;
    std::string name_;
    std::shared_ptr<Tool> tool_;
    
public:
    Operation(Type type, const std::string& name, std::shared_ptr<Tool> tool);
    virtual ~Operation() = default;
    
    // Pure virtual methods
    virtual std::unique_ptr<Toolpath> generateToolpath(const Geometry::Part& part) = 0;
    virtual bool validate() const = 0;
    
    // Getters
    Type getType() const { return type_; }
    const std::string& getName() const { return name_; }
    std::shared_ptr<Tool> getTool() const { return tool_; }
    
    // Factory method
    static std::unique_ptr<Operation> createOperation(Type type, const std::string& name, 
                                                     std::shared_ptr<Tool> tool);
};

} // namespace Toolpath
} // namespace IntuiCAM 