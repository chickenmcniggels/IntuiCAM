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
        Grooving
    };
    
    struct CuttingParameters {
        double feedRate = 0.1;          // mm/rev
        double spindleSpeed = 1000;     // RPM
        double depthOfCut = 1.0;        // mm
        double stepover = 0.5;          // mm
        double rapidFeedRate = 5000.0;  // mm/min (for rapid movements)
    };
    
    struct Geometry {
        double tipRadius = 0.4;         // mm
        double clearanceAngle = 7.0;    // degrees
        double rakeAngle = 0.0;         // degrees
        double insertWidth = 3.0;       // mm
        double diameter = 10.0;         // mm - cutting diameter
        double length = 50.0;           // mm - tool length
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
    
    // Additional convenience getters
    double getDiameter() const { return geometry_.diameter; }
    double getLength() const { return geometry_.length; }
    
    void setCuttingParameters(const CuttingParameters& params) { cuttingParams_ = params; }
    void setGeometry(const Geometry& geom) { geometry_ = geom; }
    
    // Convenience setters for geometry
    void setDiameter(double diameter) { geometry_.diameter = diameter; }
    void setLength(double length) { geometry_.length = length; }
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

// Additional enum for display compatibility
enum class MoveType {
    Rapid,          // G0 - rapid positioning
    Feed,           // G1 - linear feed interpolation
    Cut,            // Cutting moves
    Plunge,         // Plunge moves
    Linear = Feed,  // Alias for compatibility
    CircularCW,     // G2 - circular interpolation clockwise
    CircularCCW,    // G3 - circular interpolation counter-clockwise
    Dwell,          // G4 - dwell/pause
    ToolChange      // Tool change operation
};

// Individual toolpath movement
struct Movement {
    MovementType type;
    Geometry::Point3D position;
    Geometry::Point3D startPoint;  // Starting position of movement
    Geometry::Point3D endPoint;    // Ending position of movement
    double feedRate = 0.0;
    double spindleSpeed = 0.0;
    std::string comment;
    
    Movement(MovementType t, const Geometry::Point3D& pos) 
        : type(t), position(pos), startPoint(pos), endPoint(pos) {}
        
    Movement(MovementType t, const Geometry::Point3D& start, const Geometry::Point3D& end) 
        : type(t), position(end), startPoint(start), endPoint(end) {}
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
    void addThreadingMove(const Geometry::Point3D& position, double feedRate, double pitch);
    void addDwell(double seconds);
    
    // Getters
    const std::vector<Movement>& getMovements() const { return movements_; }
    const std::vector<Movement>& getMoves() const { return movements_; } // Alias for compatibility
    std::shared_ptr<Tool> getTool() const { return tool_; }
    const std::string& getName() const { return name_; }
    
    // Analysis
    size_t getMovementCount() const { return movements_.size(); }
    size_t getPointCount() const { return movements_.size(); }
    double estimateMachiningTime() const;
    Geometry::BoundingBox getBoundingBox() const;
    
    // Optimization
    void optimizeToolpath();
    void removeRedundantMoves();
    
    // Apply a 4x4 transform to every movement (e.g., part positioning in world space)
    void applyTransform(const Geometry::Matrix4x4& mat);
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
        Grooving
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