#include <IntuiCAM/Toolpath/Types.h>

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
    // Note: Center point would need to be stored in Movement structure for full implementation
    movements_.push_back(move);
}

void Toolpath::addThreadingMove(const Geometry::Point3D& position, double feedRate, double pitch) {
    Movement move(MovementType::Linear, position);
    move.feedRate = feedRate;
    move.comment = "Threading move, pitch: " + std::to_string(pitch);
    movements_.push_back(move);
}

void Toolpath::addDwell(double seconds) {
    Movement move(MovementType::Dwell, Geometry::Point3D(0, 0, 0));
    move.comment = "Dwell " + std::to_string(seconds) + " seconds";
    movements_.push_back(move);
}

double Toolpath::estimateMachiningTime() const {
    // Simple estimation based on movement distances and feed rates
    double totalTime = 0.0;
    for (const auto& move : movements_) {
        if (move.type == MovementType::Linear || move.type == MovementType::CircularCW || move.type == MovementType::CircularCCW) {
            // Distance-based time calculation would need previous position
            totalTime += 1.0; // Placeholder
        } else if (move.type == MovementType::Dwell) {
            totalTime += 1.0; // Extract from comment
        }
    }
    return totalTime;
}

Geometry::BoundingBox Toolpath::getBoundingBox() const {
    if (movements_.empty()) {
        return Geometry::BoundingBox();
    }
    
    auto firstPos = movements_[0].position;
    double minX = firstPos.x, maxX = firstPos.x;
    double minY = firstPos.y, maxY = firstPos.y;
    double minZ = firstPos.z, maxZ = firstPos.z;
    
    for (const auto& move : movements_) {
        const auto& pos = move.position;
        minX = std::min(minX, pos.x);
        maxX = std::max(maxX, pos.x);
        minY = std::min(minY, pos.y);
        maxY = std::max(maxY, pos.y);
        minZ = std::min(minZ, pos.z);
        maxZ = std::max(maxZ, pos.z);
    }
    
    return Geometry::BoundingBox(
        Geometry::Point3D(minX, minY, minZ),
        Geometry::Point3D(maxX, maxY, maxZ)
    );
}

void Toolpath::optimizeToolpath() {
    removeRedundantMoves();
}

void Toolpath::removeRedundantMoves() {
    if (movements_.size() < 2) return;
    
    std::vector<Movement> optimized;
    optimized.push_back(movements_[0]);
    
    for (size_t i = 1; i < movements_.size(); ++i) {
        const auto& prev = movements_[i-1];
        const auto& curr = movements_[i];
        
        // Remove consecutive moves to the same position
        if (prev.position.x != curr.position.x || 
            prev.position.y != curr.position.y || 
            prev.position.z != curr.position.z) {
            optimized.push_back(curr);
        }
    }
    
    movements_ = std::move(optimized);
}

void Toolpath::applyTransform(const Geometry::Matrix4x4& mat) {
    for (auto& move : movements_) {
        // Transform position - simplified matrix multiplication
        // Full implementation would require proper 4x4 matrix math
        move.position = move.position; // Placeholder
    }
}

// Operation implementation  
Operation::Operation(Type type, const std::string& name, std::shared_ptr<Tool> tool)
    : type_(type), name_(name), tool_(tool) {
}

std::unique_ptr<Operation> Operation::createOperation(Type type, const std::string& name, 
                                                     std::shared_ptr<Tool> tool) {
    // This would create specific operation types
    // For now, return nullptr as this is a factory method
    return nullptr;
}

} // namespace Toolpath
} // namespace IntuiCAM
