#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <IntuiCAM/Common/Types.h>

namespace IntuiCAM {
namespace Geometry {

// Forward declarations
class GeometricEntity;
class Part;
class Mesh;

// Basic geometric primitives
struct Point3D {
    double x, y, z;
    
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

struct Vector3D {
    double x, y, z;
    
    Vector3D() : x(0), y(0), z(0) {}
    Vector3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    Vector3D normalized() const;
    double magnitude() const;
};

struct Matrix4x4 {
    double data[16];
    
    Matrix4x4();
    static Matrix4x4 identity();
    static Matrix4x4 translation(const Vector3D& t);
    static Matrix4x4 rotation(const Vector3D& axis, double angle);
};

struct BoundingBox {
    Point3D min, max;
    
    BoundingBox() = default;
    BoundingBox(const Point3D& min_, const Point3D& max_) : min(min_), max(max_) {}
    
    bool contains(const Point3D& point) const;
    bool intersects(const BoundingBox& other) const;
    Vector3D size() const;
    Point3D center() const;
};

// Base class for all geometric objects
class GeometricEntity {
public:
    virtual ~GeometricEntity() = default;
    virtual BoundingBox getBoundingBox() const = 0;
    virtual std::unique_ptr<GeometricEntity> clone() const = 0;
};

// Triangulated geometry for visualization and simulation
class Mesh : public GeometricEntity {
public:
    struct Triangle {
        Point3D vertices[3];
        Vector3D normal;
    };
    
    std::vector<Triangle> triangles;
    
    BoundingBox getBoundingBox() const override;
    std::unique_ptr<GeometricEntity> clone() const override;
    
    // Mesh operations
    void addTriangle(const Triangle& triangle);
    size_t getTriangleCount() const { return triangles.size(); }
    double calculateVolume() const;
};

// Complete part or assembly with topology
class Part : public GeometricEntity {
public:
    Part() = default;
    virtual ~Part() = default;
    
    // Part properties
    virtual double getVolume() const = 0;
    virtual double getSurfaceArea() const = 0;
    virtual std::unique_ptr<Mesh> generateMesh(double tolerance = 0.1) const = 0;
    
    // Geometric queries
    virtual std::vector<Point3D> detectCylindricalFeatures() const = 0;
    virtual std::optional<double> getLargestCylinderDiameter() const = 0;
};

} // namespace Geometry
} // namespace IntuiCAM 