#pragma once

#include <memory>
#include <vector>
#include <IntuiCAM/Geometry/Types.h>
#include <IntuiCAM/Toolpath/Types.h>

namespace IntuiCAM {
namespace Simulation {

// Material removal simulation and verification
class MaterialSimulator {
public:
    struct SimulationSettings {
        double voxelSize = 0.1;             // mm - resolution of simulation
        bool enableCollisionDetection = true;
        bool enableVisualization = true;
        double simulationSpeed = 1.0;       // 1.0 = real-time
        bool showToolPath = true;
        bool showMaterialRemoval = true;
    };
    
    struct SimulationResult {
        bool success = false;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        double totalMachiningTime = 0.0;    // minutes
        double materialRemoved = 0.0;       // cubic mm
        std::unique_ptr<Geometry::Mesh> finalPartMesh;
        std::vector<Geometry::Point3D> collisionPoints;
    };
    
private:
    SimulationSettings settings_;
    std::unique_ptr<Geometry::Part> stockMaterial_;
    std::unique_ptr<Geometry::Part> chuckGeometry_;
    
public:
    MaterialSimulator(const SimulationSettings& settings = SimulationSettings{});
    
    // Setup
    void setStockMaterial(std::unique_ptr<Geometry::Part> stock);
    void setChuckGeometry(std::unique_ptr<Geometry::Part> chuck);
    void setSettings(const SimulationSettings& settings) { settings_ = settings; }
    
    // Simulation
    SimulationResult simulate(const std::vector<std::shared_ptr<Toolpath::Toolpath>>& toolpaths);
    SimulationResult simulate(const Toolpath::Toolpath& toolpath);
    
    // Step-by-step simulation
    void startSimulation(const Toolpath::Toolpath& toolpath);
    bool stepSimulation();  // Returns false when complete
    void pauseSimulation();
    void resetSimulation();
    
    // Analysis
    std::vector<Geometry::Point3D> detectCollisions(const Toolpath::Toolpath& toolpath) const;
    double calculateMachiningTime(const Toolpath::Toolpath& toolpath) const;
    double calculateMaterialRemovalRate(const Toolpath::Toolpath& toolpath) const;
    
    // Visualization
    std::unique_ptr<Geometry::Mesh> getCurrentStateMesh() const;
    std::unique_ptr<Geometry::Mesh> getToolMesh(const Toolpath::Tool& tool, 
                                               const Geometry::Point3D& position) const;
};

// Collision detection system
class CollisionDetector {
public:
    enum class CollisionType {
        ToolChuck,      // Tool collides with chuck
        ToolStock,      // Tool collides with remaining stock
        ToolTailstock,  // Tool collides with tailstock
        RapidMove       // Rapid move through material
    };
    
    struct Collision {
        CollisionType type;
        Geometry::Point3D location;
        double severity;        // 0.0 to 1.0
        std::string description;
        size_t movementIndex;   // Index in toolpath
    };
    
private:
    std::unique_ptr<Geometry::Part> chuckGeometry_;
    std::unique_ptr<Geometry::Part> stockGeometry_;
    std::unique_ptr<Geometry::Part> tailstockGeometry_;
    
public:
    CollisionDetector();
    
    // Setup
    void setChuckGeometry(std::unique_ptr<Geometry::Part> chuck);
    void setStockGeometry(std::unique_ptr<Geometry::Part> stock);
    void setTailstockGeometry(std::unique_ptr<Geometry::Part> tailstock);
    
    // Detection
    std::vector<Collision> detectCollisions(const Toolpath::Toolpath& toolpath) const;
    bool hasCollisions(const Toolpath::Toolpath& toolpath) const;
    
    // Individual checks
    bool checkToolChuckCollision(const Toolpath::Tool& tool, 
                                const Geometry::Point3D& position) const;
    bool checkRapidMoveCollision(const Geometry::Point3D& start, 
                                const Geometry::Point3D& end) const;
};

// Visualization mesh output for simulation
class SimulationVisualizer {
public:
    struct VisualizationOptions {
        bool showStock = true;
        bool showChuck = true;
        bool showTool = true;
        bool showToolpath = true;
        bool showCuttingArea = true;
        double toolTransparency = 0.7;
        double stockTransparency = 0.3;
    };
    
private:
    VisualizationOptions options_;
    
public:
    SimulationVisualizer(const VisualizationOptions& options = VisualizationOptions{});
    
    // Mesh generation
    std::unique_ptr<Geometry::Mesh> generateStockMesh(const Geometry::Part& stock) const;
    std::unique_ptr<Geometry::Mesh> generateChuckMesh(const Geometry::Part& chuck) const;
    std::unique_ptr<Geometry::Mesh> generateToolMesh(const Toolpath::Tool& tool,
                                                     const Geometry::Point3D& position) const;
    std::unique_ptr<Geometry::Mesh> generateToolpathMesh(const Toolpath::Toolpath& toolpath) const;
    
    // Animation support
    std::vector<std::unique_ptr<Geometry::Mesh>> generateAnimationFrames(
        const Toolpath::Toolpath& toolpath, 
        const MaterialSimulator& simulator,
        int frameCount = 100) const;
    
    // Settings
    void setOptions(const VisualizationOptions& options) { options_ = options; }
    const VisualizationOptions& getOptions() const { return options_; }
};

// Voxel-based material representation for simulation
class VoxelGrid {
public:
    enum class VoxelState {
        Empty,      // No material
        Material,   // Stock material
        Chuck,      // Chuck geometry (immovable)
        Removed     // Material removed by cutting
    };
    
private:
    std::vector<std::vector<std::vector<VoxelState>>> grid_;
    Geometry::BoundingBox bounds_;
    double voxelSize_;
    
public:
    VoxelGrid(const Geometry::BoundingBox& bounds, double voxelSize);
    
    // Grid operations
    void setVoxel(int x, int y, int z, VoxelState state);
    VoxelState getVoxel(int x, int y, int z) const;
    void fillFromGeometry(const Geometry::Part& part, VoxelState state);
    
    // Material removal
    void removeMaterial(const Geometry::Point3D& center, double radius);
    void removeMaterialAlongPath(const Geometry::Point3D& start, 
                                const Geometry::Point3D& end, 
                                double toolRadius);
    
    // Analysis
    double calculateVolume(VoxelState state) const;
    std::unique_ptr<Geometry::Mesh> generateMesh(VoxelState state) const;
    
    // Conversion
    Geometry::Point3D voxelToWorld(int x, int y, int z) const;
    void worldToVoxel(const Geometry::Point3D& world, int& x, int& y, int& z) const;
};

} // namespace Simulation
} // namespace IntuiCAM 