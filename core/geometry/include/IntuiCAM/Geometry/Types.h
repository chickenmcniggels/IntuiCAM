#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <IntuiCAM/Common/Types.h>

// Forward declare OpenCASCADE types
class TopoDS_Shape;

namespace IntuiCAM {
namespace Geometry {

// Forward declarations
class GeometricEntity;
class Part;
class Mesh;
struct Matrix4x4;

// Basic geometric primitives
struct Point3D {
    double x, y, z;
    
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    // Transform point by matrix
    Point3D transform(const Matrix4x4& mat) const;
};

struct Vector3D {
    double x, y, z;
    
    Vector3D() : x(0), y(0), z(0) {}
    Vector3D(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    Vector3D normalized() const;
    double magnitude() const;
    
    // Transform vector by matrix (direction only, no translation)
    Vector3D transform(const Matrix4x4& mat) const;
};

struct Matrix4x4 {
    double data[16];
    
    Matrix4x4();
    static Matrix4x4 identity();
    static Matrix4x4 translation(const Vector3D& t);
    static Matrix4x4 rotation(const Vector3D& axis, double angle);
    
    // Matrix operations
    Matrix4x4 operator*(const Matrix4x4& other) const;
    Matrix4x4 inverse() const;
    Point3D transformPoint(const Point3D& point) const;
    Vector3D transformVector(const Vector3D& vector) const;
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

struct Point2D {
    double x; // radius (or X in profile plane)
    double z; // axial position
    Point2D() : x(0.0), z(0.0) {}
    Point2D(double x_, double z_) : x(x_), z(z_) {}
};

// Work Coordinate System for lathe operations
class WorkCoordinateSystem {
public:
    enum class Type {
        Global,     // Global viewer coordinate system
        WorkG54,    // Work coordinate system (G54) - origin at raw material end
        Machine     // Machine coordinate system
    };
    
private:
    Type type_;
    Point3D origin_;          // Origin of this coordinate system in global coordinates
    Vector3D xAxis_;          // X-axis direction (radial in lathe)
    Vector3D yAxis_;          // Y-axis direction (unused in 2D lathe operations)
    Vector3D zAxis_;          // Z-axis direction (spindle axis in lathe)
    Matrix4x4 toGlobal_;      // Transform from this CS to global
    Matrix4x4 fromGlobal_;    // Transform from global to this CS
    
public:
    WorkCoordinateSystem(Type type = Type::Global);
    
    // Setup methods
    void setOrigin(const Point3D& origin);
    void setAxes(const Vector3D& xAxis, const Vector3D& yAxis, const Vector3D& zAxis);
    void setFromLatheMaterial(const Point3D& rawMaterialEnd, const Vector3D& spindleAxis);
    
    // Coordinate transformations
    Point3D toGlobal(const Point3D& localPoint) const;
    Point3D fromGlobal(const Point3D& globalPoint) const;
    Vector3D toGlobal(const Vector3D& localVector) const;
    Vector3D fromGlobal(const Vector3D& globalVector) const;
    
    // Matrix access
    const Matrix4x4& getToGlobalMatrix() const { return toGlobal_; }
    const Matrix4x4& getFromGlobalMatrix() const { return fromGlobal_; }
    
    // Properties
    Type getType() const { return type_; }
    const Point3D& getOrigin() const { return origin_; }
    const Vector3D& getXAxis() const { return xAxis_; }
    const Vector3D& getYAxis() const { return yAxis_; }
    const Vector3D& getZAxis() const { return zAxis_; }
    
    // Lathe-specific convenience methods
    Point2D globalToLathe(const Point3D& globalPoint) const;  // Convert to (X=radius, Z=axial)
    Point3D latheToGlobal(const Point2D& lathePoint) const;   // Convert from (X=radius, Z=axial)
    
private:
    void updateTransformMatrices();
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

// OpenCASCADE-based implementation of Part
class OCCTPart : public Part {
private:
    void* m_shape; // Actually TopoDS_Shape but stored as void* to avoid header dependencies
    mutable BoundingBox m_boundingBox;
    mutable bool m_boundingBoxComputed = false;

public:
    OCCTPart(const void* shape);
    virtual ~OCCTPart();

    // Part interface implementation
    BoundingBox getBoundingBox() const override;
    std::unique_ptr<GeometricEntity> clone() const override;
    double getVolume() const override;
    double getSurfaceArea() const override;
    std::unique_ptr<Mesh> generateMesh(double tolerance = 0.1) const override;
    
    // Cylinder detection
    std::vector<Point3D> detectCylindricalFeatures() const override;
    std::optional<double> getLargestCylinderDiameter() const override;
    
    // OCC-specific methods
    const TopoDS_Shape& getOCCTShape() const;
    void setOCCTShape(const TopoDS_Shape& shape);
};

} // namespace Geometry
} // namespace IntuiCAM 