#include <IntuiCAM/Toolpath/Types.h>
#include <cmath>
#include <algorithm>

namespace IntuiCAM {
namespace Toolpath {

// Tool Implementation
Tool::Tool(Type type, const std::string& name) 
    : type_(type), name_(name) {
    // Set default parameters based on tool type
    switch (type) {
        case Type::Turning:
            geometry_.diameter = 12.0;
            geometry_.tipRadius = 0.4;
            cuttingParams_.feedRate = 0.15;
            cuttingParams_.spindleSpeed = 1200;
            cuttingParams_.depthOfCut = 1.5;
            break;
        case Type::Facing:
            geometry_.diameter = 16.0;
            geometry_.tipRadius = 0.8;
            cuttingParams_.feedRate = 0.2;
            cuttingParams_.spindleSpeed = 800;
            cuttingParams_.depthOfCut = 1.0;
            break;
        case Type::Parting:
            geometry_.diameter = 3.0;
            geometry_.tipRadius = 0.0;
            cuttingParams_.feedRate = 0.05;
            cuttingParams_.spindleSpeed = 600;
            cuttingParams_.depthOfCut = 0.5;
            break;
        case Type::Threading:
            geometry_.diameter = 8.0;
            geometry_.tipRadius = 0.0;
            cuttingParams_.feedRate = 1.0; // Pitch-dependent
            cuttingParams_.spindleSpeed = 400;
            cuttingParams_.depthOfCut = 0.2;
            break;
        case Type::Grooving:
            geometry_.diameter = 4.0;
            geometry_.tipRadius = 0.1;
            cuttingParams_.feedRate = 0.08;
            cuttingParams_.spindleSpeed = 800;
            cuttingParams_.depthOfCut = 0.8;
            break;
    }
}

// Toolpath Implementation
Toolpath::Toolpath(const std::string& name, std::shared_ptr<Tool> tool, OperationType opType) 
    : name_(name), tool_(tool), operationType_(opType) {}

void Toolpath::addMovement(const Movement& movement) {
    movements_.push_back(movement);
}

void Toolpath::addRapidMove(const Geometry::Point3D& position) {
    Movement move(MovementType::Rapid, movements_.empty() ? position : movements_.back().position, position);
    move.operationType = operationType_;
    movements_.push_back(move);
}

void Toolpath::addLinearMove(const Geometry::Point3D& position, double feedRate) {
    Movement move(MovementType::Linear, movements_.empty() ? position : movements_.back().position, position);
    move.feedRate = feedRate;
    move.operationType = operationType_;
    movements_.push_back(move);
}

void Toolpath::addCircularMove(const Geometry::Point3D& position, const Geometry::Point3D& center, 
                               bool clockwise, double feedRate) {
    MovementType type = clockwise ? MovementType::CircularCW : MovementType::CircularCCW;
    Movement move(type, movements_.empty() ? position : movements_.back().position, position);
    move.feedRate = feedRate;
    move.operationType = operationType_;
    movements_.push_back(move);
}

void Toolpath::addThreadingMove(const Geometry::Point3D& position, double feedRate, double pitch) {
    Movement move(MovementType::Linear, movements_.empty() ? position : movements_.back().position, position);
    move.feedRate = feedRate;
    move.operationType = OperationType::Threading;
    move.comment = "Threading pitch: " + std::to_string(pitch);
    movements_.push_back(move);
}

void Toolpath::addDwell(double seconds) {
    Movement move(MovementType::Dwell, movements_.empty() ? Geometry::Point3D(0,0,0) : movements_.back().position);
    move.operationType = operationType_;
    move.comment = "Dwell " + std::to_string(seconds) + "s";
    movements_.push_back(move);
}

// Movement operations with operation context
void Toolpath::addRapidMove(const Geometry::Point3D& position, OperationType opType, const std::string& opName) {
    Movement move(MovementType::Rapid, movements_.empty() ? position : movements_.back().position, position);
    move.operationType = opType;
    move.operationName = opName;
    movements_.push_back(move);
}

void Toolpath::addLinearMove(const Geometry::Point3D& position, double feedRate, OperationType opType, const std::string& opName) {
    Movement move(MovementType::Linear, movements_.empty() ? position : movements_.back().position, position);
    move.feedRate = feedRate;
    move.operationType = opType;
    move.operationName = opName;
    movements_.push_back(move);
}

void Toolpath::addCircularMove(const Geometry::Point3D& position, const Geometry::Point3D& center, 
                              bool clockwise, double feedRate, OperationType opType, const std::string& opName) {
    MovementType type = clockwise ? MovementType::CircularCW : MovementType::CircularCCW;
    Movement move(type, movements_.empty() ? position : movements_.back().position, position);
    move.feedRate = feedRate;
    move.operationType = opType;
    move.operationName = opName;
    movements_.push_back(move);
}

double Toolpath::estimateMachiningTime() const {
    double totalTime = 0.0;
    
    for (const auto& movement : movements_) {
        if (movement.type == MovementType::Rapid) {
            // Rapid moves are fast
            double distance = std::sqrt(
                std::pow(movement.endPoint.x - movement.startPoint.x, 2) +
                std::pow(movement.endPoint.y - movement.startPoint.y, 2) +
                std::pow(movement.endPoint.z - movement.startPoint.z, 2)
            );
            totalTime += distance / (tool_ ? tool_->getCuttingParameters().rapidFeedRate : 5000.0);
        } else if (movement.feedRate > 0.0) {
            // Calculate based on feed rate
            double distance = std::sqrt(
                std::pow(movement.endPoint.x - movement.startPoint.x, 2) +
                std::pow(movement.endPoint.y - movement.startPoint.y, 2) +
                std::pow(movement.endPoint.z - movement.startPoint.z, 2)
            );
            totalTime += distance / movement.feedRate;
        }
    }
    
    return totalTime;
}

Geometry::BoundingBox Toolpath::getBoundingBox() const {
    if (movements_.empty()) {
        return Geometry::BoundingBox();
    }
    
    double minX = movements_[0].position.x, maxX = movements_[0].position.x;
    double minY = movements_[0].position.y, maxY = movements_[0].position.y;
    double minZ = movements_[0].position.z, maxZ = movements_[0].position.z;
    
    for (const auto& movement : movements_) {
        minX = std::min(minX, movement.position.x);
        maxX = std::max(maxX, movement.position.x);
        minY = std::min(minY, movement.position.y);
        maxY = std::max(maxY, movement.position.y);
        minZ = std::min(minZ, movement.position.z);
        maxZ = std::max(maxZ, movement.position.z);
    }
    
    return Geometry::BoundingBox(
        Geometry::Point3D(minX, minY, minZ),
        Geometry::Point3D(maxX, maxY, maxZ)
    );
}

void Toolpath::optimizeToolpath() {
    // Remove redundant movements (same position)
    removeRedundantMoves();
    
    // Additional optimization could be added here
}

void Toolpath::removeRedundantMoves() {
    if (movements_.size() < 2) return;
    
    std::vector<Movement> optimized;
    optimized.reserve(movements_.size());
    optimized.push_back(movements_[0]);
    
    for (size_t i = 1; i < movements_.size(); ++i) {
        const auto& current = movements_[i];
        const auto& previous = optimized.back();
        
        // Check if positions are significantly different
        double distance = std::sqrt(
            std::pow(current.position.x - previous.position.x, 2) +
            std::pow(current.position.y - previous.position.y, 2) +
            std::pow(current.position.z - previous.position.z, 2)
        );
        
        // Only add if distance is above tolerance
        if (distance > 0.001) { // 1 micron tolerance
            optimized.push_back(current);
        }
    }
    
    movements_ = std::move(optimized);
}

void Toolpath::applyTransform(const Geometry::Matrix4x4& mat) {
    for (auto& movement : movements_) {
        // Manual matrix multiplication since transformPoint doesn't exist yet
        // [x' y' z' 1] = [x y z 1] * [4x4 matrix]
        
        // Transform position
        double x = movement.position.x * mat.data[0] + movement.position.y * mat.data[4] + movement.position.z * mat.data[8] + mat.data[12];
        double y = movement.position.x * mat.data[1] + movement.position.y * mat.data[5] + movement.position.z * mat.data[9] + mat.data[13];
        double z = movement.position.x * mat.data[2] + movement.position.y * mat.data[6] + movement.position.z * mat.data[10] + mat.data[14];
        movement.position = Geometry::Point3D(x, y, z);
        
        // Transform start point
        x = movement.startPoint.x * mat.data[0] + movement.startPoint.y * mat.data[4] + movement.startPoint.z * mat.data[8] + mat.data[12];
        y = movement.startPoint.x * mat.data[1] + movement.startPoint.y * mat.data[5] + movement.startPoint.z * mat.data[9] + mat.data[13];
        z = movement.startPoint.x * mat.data[2] + movement.startPoint.y * mat.data[6] + movement.startPoint.z * mat.data[10] + mat.data[14];
        movement.startPoint = Geometry::Point3D(x, y, z);
        
        // Transform end point
        x = movement.endPoint.x * mat.data[0] + movement.endPoint.y * mat.data[4] + movement.endPoint.z * mat.data[8] + mat.data[12];
        y = movement.endPoint.x * mat.data[1] + movement.endPoint.y * mat.data[5] + movement.endPoint.z * mat.data[9] + mat.data[13];
        z = movement.endPoint.x * mat.data[2] + movement.endPoint.y * mat.data[6] + movement.endPoint.z * mat.data[10] + mat.data[14];
        movement.endPoint = Geometry::Point3D(x, y, z);
    }
}

// Operation Implementation
Operation::Operation(Type type, const std::string& name, std::shared_ptr<Tool> tool)
    : type_(type), name_(name), tool_(tool) {}

std::unique_ptr<Operation> Operation::createOperation(Type type, const std::string& name, 
                                                     std::shared_ptr<Tool> tool) {
    // This would create specific operation types
    // For now, return nullptr as this is factory method placeholder
    return nullptr;
}

// Utility functions for operation type mapping
std::string operationTypeToString(OperationType type) {
    switch (type) {
        case OperationType::Facing: return "Facing";
        case OperationType::ExternalRoughing: return "External Roughing";
        case OperationType::InternalRoughing: return "Internal Roughing";
        case OperationType::ExternalFinishing: return "External Finishing";
        case OperationType::InternalFinishing: return "Internal Finishing";
        case OperationType::Drilling: return "Drilling";
        case OperationType::Boring: return "Boring";
        case OperationType::ExternalGrooving: return "External Grooving";
        case OperationType::InternalGrooving: return "Internal Grooving";
        case OperationType::Chamfering: return "Chamfering";
        case OperationType::Threading: return "Threading";
        case OperationType::Parting: return "Parting";
        case OperationType::Unknown:
        default: return "Unknown";
    }
}

OperationType stringToOperationType(const std::string& str) {
    if (str == "Facing") return OperationType::Facing;
    if (str == "External Roughing") return OperationType::ExternalRoughing;
    if (str == "Internal Roughing") return OperationType::InternalRoughing;
    if (str == "External Finishing") return OperationType::ExternalFinishing;
    if (str == "Internal Finishing") return OperationType::InternalFinishing;
    if (str == "Drilling") return OperationType::Drilling;
    if (str == "Boring") return OperationType::Boring;
    if (str == "External Grooving") return OperationType::ExternalGrooving;
    if (str == "Internal Grooving") return OperationType::InternalGrooving;
    if (str == "Chamfering") return OperationType::Chamfering;
    if (str == "Threading") return OperationType::Threading;
    if (str == "Parting") return OperationType::Parting;
    return OperationType::Unknown;
}

} // namespace Toolpath
} // namespace IntuiCAM
