#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Toolpath/Operations.h>
#include <algorithm>
#include <cmath>

namespace IntuiCAM {
namespace Toolpath {

// Tool implementation
Tool::Tool(Type type, const std::string& name) 
    : type_(type), name_(name) {
}

// Toolpath implementation
Toolpath::Toolpath(const std::string& name, std::shared_ptr<Tool> tool)
    : name_(name), tool_(tool) {
}

void Toolpath::addMovement(const Movement& movement) {
    movements_.push_back(movement);
}

void Toolpath::addRapidMove(const Geometry::Point3D& position) {
    movements_.emplace_back(MovementType::Rapid, position);
}

void Toolpath::addLinearMove(const Geometry::Point3D& position, double feedRate) {
    Movement move(MovementType::Linear, position);
    move.feedRate = feedRate;
    movements_.push_back(move);
}

void Toolpath::addCircularMove(const Geometry::Point3D& position, const Geometry::Point3D& center, 
                              bool clockwise, double feedRate) {
    MovementType type = clockwise ? MovementType::CircularCW : MovementType::CircularCCW;
    Movement move(type, position);
    move.feedRate = feedRate;
    movements_.push_back(move);
}

void Toolpath::addDwell(double seconds) {
    Movement move(MovementType::Dwell, Geometry::Point3D(0, 0, 0));
    move.comment = "Dwell " + std::to_string(seconds) + " seconds";
    movements_.push_back(move);
}

void Toolpath::appendToolpath(const Toolpath& other) {
    for (const auto& m : other.getMovements()) {
        movements_.push_back(m);
    }
}

double Toolpath::estimateMachiningTime() const {
    double totalTime = 0.0;
    
    for (size_t i = 1; i < movements_.size(); ++i) {
        const auto& prev = movements_[i-1];
        const auto& curr = movements_[i];
        
        if (curr.type == MovementType::Linear || curr.type == MovementType::CircularCW || 
            curr.type == MovementType::CircularCCW) {
            
            // Calculate distance
            double dx = curr.position.x - prev.position.x;
            double dy = curr.position.y - prev.position.y;
            double dz = curr.position.z - prev.position.z;
            double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
            
            // Estimate time based on feed rate
            if (curr.feedRate > 0.0) {
                totalTime += distance / curr.feedRate; // minutes
            }
        }
    }
    
    return totalTime;
}

Geometry::BoundingBox Toolpath::getBoundingBox() const {
    if (movements_.empty()) {
        return Geometry::BoundingBox();
    }
    
    Geometry::Point3D minPoint = movements_[0].position;
    Geometry::Point3D maxPoint = movements_[0].position;
    
    for (const auto& movement : movements_) {
        const auto& pos = movement.position;
        minPoint.x = std::min(minPoint.x, pos.x);
        minPoint.y = std::min(minPoint.y, pos.y);
        minPoint.z = std::min(minPoint.z, pos.z);
        maxPoint.x = std::max(maxPoint.x, pos.x);
        maxPoint.y = std::max(maxPoint.y, pos.y);
        maxPoint.z = std::max(maxPoint.z, pos.z);
    }
    
    return Geometry::BoundingBox(minPoint, maxPoint);
}

void Toolpath::optimizeToolpath() {
    // Basic optimization - remove redundant moves
    removeRedundantMoves();
}

void Toolpath::removeRedundantMoves() {
    if (movements_.size() < 2) return;
    
    std::vector<Movement> optimized;
    optimized.push_back(movements_[0]);
    
    for (size_t i = 1; i < movements_.size(); ++i) {
        const auto& prev = optimized.back();
        const auto& curr = movements_[i];
        
        // Skip if same position and type
        if (prev.type == curr.type && 
            std::abs(prev.position.x - curr.position.x) < 1e-6 &&
            std::abs(prev.position.y - curr.position.y) < 1e-6 &&
            std::abs(prev.position.z - curr.position.z) < 1e-6) {
            continue;
        }
        
        optimized.push_back(curr);
    }
    
    movements_ = std::move(optimized);
}

// Operation implementation
Operation::Operation(Type type, const std::string& name, std::shared_ptr<Tool> tool)
    : type_(type), name_(name), tool_(tool) {
}

std::unique_ptr<Operation> Operation::createOperation(Type type, const std::string& name,
                                                     std::shared_ptr<Tool> tool) {
    switch (type) {
        case Operation::Type::Facing:
            return std::make_unique<FacingOperation>(name, tool);
        case Operation::Type::Roughing:
            return std::make_unique<RoughingOperation>(name, tool);
        case Operation::Type::Finishing:
            return std::make_unique<FinishingOperation>(name, tool);
        case Operation::Type::Parting:
            return std::make_unique<PartingOperation>(name, tool);
        case Operation::Type::Threading:
            return std::make_unique<ThreadingOperation>(name, tool);
        case Operation::Type::Grooving:
            return std::make_unique<GroovingOperation>(name, tool);
        case Operation::Type::Chamfering:
            return std::make_unique<ChamferingOperation>(name, tool);
        case Operation::Type::Contouring:
            return std::make_unique<ContouringOperation>(name, tool);
        default:
            return nullptr;
    }
}

} // namespace Toolpath
} // namespace IntuiCAM 