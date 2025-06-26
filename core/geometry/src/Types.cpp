#include <IntuiCAM/Geometry/Types.h>
#include <cmath>
#include <algorithm>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopLoc_Location.hxx>
#include <Poly_Triangulation.hxx>
#include <BRepTools.hxx>

namespace IntuiCAM {
namespace Geometry {

// Vector3D implementations
Vector3D Vector3D::normalized() const {
    double mag = magnitude();
    if (mag > 0.0) {
        return Vector3D(x / mag, y / mag, z / mag);
    }
    return Vector3D(0, 0, 0);
}

double Vector3D::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

// Matrix4x4 implementations
Matrix4x4::Matrix4x4() {
    // Initialize as identity matrix
    for (int i = 0; i < 16; ++i) {
        data[i] = 0.0;
    }
    data[0] = data[5] = data[10] = data[15] = 1.0;
}

Matrix4x4 Matrix4x4::identity() {
    return Matrix4x4();
}

Matrix4x4 Matrix4x4::translation(const Vector3D& t) {
    Matrix4x4 result = identity();
    result.data[12] = t.x;
    result.data[13] = t.y;
    result.data[14] = t.z;
    return result;
}

Matrix4x4 Matrix4x4::rotation(const Vector3D& axis, double angle) {
    Matrix4x4 result = identity();
    
    Vector3D normalizedAxis = axis.normalized();
    double c = std::cos(angle);
    double s = std::sin(angle);
    double t = 1.0 - c;
    
    double x = normalizedAxis.x;
    double y = normalizedAxis.y;
    double z = normalizedAxis.z;
    
    result.data[0] = t * x * x + c;
    result.data[1] = t * x * y - s * z;
    result.data[2] = t * x * z + s * y;
    
    result.data[4] = t * x * y + s * z;
    result.data[5] = t * y * y + c;
    result.data[6] = t * y * z - s * x;
    
    result.data[8] = t * x * z - s * y;
    result.data[9] = t * y * z + s * x;
    result.data[10] = t * z * z + c;
    
    return result;
}

Vector3D Vector3D::transform(const Matrix4x4& mat) const {
    // Transform vector (direction only, no translation)
    double newX = x * mat.data[0] + y * mat.data[4] + z * mat.data[8];
    double newY = x * mat.data[1] + y * mat.data[5] + z * mat.data[9];
    double newZ = x * mat.data[2] + y * mat.data[6] + z * mat.data[10];
    return Vector3D(newX, newY, newZ);
}

Point3D Point3D::transform(const Matrix4x4& mat) const {
    return mat.transformPoint(*this);
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 result;
    
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.data[row * 4 + col] = 0.0;
            for (int k = 0; k < 4; ++k) {
                result.data[row * 4 + col] += data[row * 4 + k] * other.data[k * 4 + col];
            }
        }
    }
    
    return result;
}

Matrix4x4 Matrix4x4::inverse() const {
    Matrix4x4 result;
    double* inv = result.data;
    const double* m = data;
    
    // Calculate inverse using adjugate method
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + 
             m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - 
              m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + 
             m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - 
               m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - 
              m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + 
             m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - 
              m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + 
              m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + 
             m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - 
              m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + 
              m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - 
               m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    double det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (std::abs(det) < 1e-9) {
        return Matrix4x4::identity();
    }

    det = 1.0 / det;
    for (int i = 0; i < 16; i++) {
        inv[i] = inv[i] * det;
    }

    return result;
}

Point3D Matrix4x4::transformPoint(const Point3D& point) const {
    double x = point.x * data[0] + point.y * data[4] + point.z * data[8] + data[12];
    double y = point.x * data[1] + point.y * data[5] + point.z * data[9] + data[13];
    double z = point.x * data[2] + point.y * data[6] + point.z * data[10] + data[14];
    double w = point.x * data[3] + point.y * data[7] + point.z * data[11] + data[15];
    
    if (std::abs(w - 1.0) > 1e-9) {
        x /= w; y /= w; z /= w;
    }
    
    return Point3D(x, y, z);
}

Vector3D Matrix4x4::transformVector(const Vector3D& vector) const {
    double x = vector.x * data[0] + vector.y * data[4] + vector.z * data[8];
    double y = vector.x * data[1] + vector.y * data[5] + vector.z * data[9];
    double z = vector.x * data[2] + vector.y * data[6] + vector.z * data[10];
    return Vector3D(x, y, z);
}

// BoundingBox implementations
bool BoundingBox::contains(const Point3D& point) const {
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y ||
             max.z < other.min.z || min.z > other.max.z);
}

Vector3D BoundingBox::size() const {
    return Vector3D(max.x - min.x, max.y - min.y, max.z - min.z);
}

Point3D BoundingBox::center() const {
    return Point3D((min.x + max.x) / 2.0, (min.y + max.y) / 2.0, (min.z + max.z) / 2.0);
}

// Mesh implementations
BoundingBox Mesh::getBoundingBox() const {
    if (triangles.empty()) {
        return BoundingBox();
    }
    
    Point3D minPoint = triangles[0].vertices[0];
    Point3D maxPoint = triangles[0].vertices[0];
    
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            const Point3D& vertex = triangle.vertices[i];
            minPoint.x = std::min(minPoint.x, vertex.x);
            minPoint.y = std::min(minPoint.y, vertex.y);
            minPoint.z = std::min(minPoint.z, vertex.z);
            maxPoint.x = std::max(maxPoint.x, vertex.x);
            maxPoint.y = std::max(maxPoint.y, vertex.y);
            maxPoint.z = std::max(maxPoint.z, vertex.z);
        }
    }
    
    return BoundingBox(minPoint, maxPoint);
}

std::unique_ptr<GeometricEntity> Mesh::clone() const {
    auto clonedMesh = std::make_unique<Mesh>();
    clonedMesh->triangles = this->triangles;
    return clonedMesh;
}

void Mesh::addTriangle(const Triangle& triangle) {
    triangles.push_back(triangle);
}

double Mesh::calculateVolume() const {
    double volume = 0.0;
    
    for (const auto& triangle : triangles) {
        const Point3D& v0 = triangle.vertices[0];
        const Point3D& v1 = triangle.vertices[1];
        const Point3D& v2 = triangle.vertices[2];
        
        // Calculate volume contribution using divergence theorem
        volume += (v0.x * (v1.y * v2.z - v2.y * v1.z) +
                   v1.x * (v2.y * v0.z - v0.y * v2.z) +
                   v2.x * (v0.y * v1.z - v1.y * v0.z)) / 6.0;
    }
    
    return std::abs(volume);
}

// OCCTPart implementation
OCCTPart::OCCTPart(const void* shape) : m_shape(nullptr), m_boundingBoxComputed(false) {
    if (shape) {
        const TopoDS_Shape* occShape = static_cast<const TopoDS_Shape*>(shape);
        m_shape = new TopoDS_Shape(*occShape);
    }
}

OCCTPart::~OCCTPart() {
    if (m_shape) {
        delete static_cast<TopoDS_Shape*>(m_shape);
        m_shape = nullptr;
    }
}

BoundingBox OCCTPart::getBoundingBox() const {
    if (!m_boundingBoxComputed) {
        // Compute bounding box using OpenCASCADE
        Bnd_Box occBBox;
        BRepBndLib::Add(*static_cast<TopoDS_Shape*>(m_shape), occBBox);
        
        double xmin, ymin, zmin, xmax, ymax, zmax;
        occBBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        m_boundingBox.min = Point3D(xmin, ymin, zmin);
        m_boundingBox.max = Point3D(xmax, ymax, zmax);
        
        m_boundingBoxComputed = true;
    }
    
    return m_boundingBox;
}

std::unique_ptr<GeometricEntity> OCCTPart::clone() const {
    return std::make_unique<OCCTPart>(m_shape);
}

double OCCTPart::getVolume() const {
    GProp_GProps volumeProps;
    BRepGProp::VolumeProperties(*static_cast<TopoDS_Shape*>(m_shape), volumeProps);
    return volumeProps.Mass();
}

double OCCTPart::getSurfaceArea() const {
    GProp_GProps surfaceProps;
    BRepGProp::SurfaceProperties(*static_cast<TopoDS_Shape*>(m_shape), surfaceProps);
    return surfaceProps.Mass();
}

std::unique_ptr<Mesh> OCCTPart::generateMesh(double tolerance) const {
    auto mesh = std::make_unique<Mesh>();
    
    if (!m_shape) {
        return mesh;
    }
    
    try {
        const TopoDS_Shape* shape = static_cast<const TopoDS_Shape*>(m_shape);
        
        // Generate mesh using OpenCASCADE meshing algorithms
        BRepMesh_IncrementalMesh mesher(*shape, tolerance);
        
        if (mesher.IsDone()) {
            // Extract triangulation from all faces
            TopExp_Explorer faceExplorer(*shape, TopAbs_FACE);
            
            for (; faceExplorer.More(); faceExplorer.Next()) {
                const TopoDS_Face& face = TopoDS::Face(faceExplorer.Current());
                TopLoc_Location location;
                
                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
                
                if (!triangulation.IsNull()) {
                    // Extract triangles from the triangulation
                    for (int i = 1; i <= triangulation->NbTriangles(); ++i) {
                        const Poly_Triangle& triangle = triangulation->Triangle(i);
                        
                        int n1, n2, n3;
                        triangle.Get(n1, n2, n3);
                        
                        // Get vertices (1-based indexing in OpenCASCADE)
                        gp_Pnt p1 = triangulation->Node(n1);
                        gp_Pnt p2 = triangulation->Node(n2);
                        gp_Pnt p3 = triangulation->Node(n3);
                        
                        // Apply location transformation if needed
                        if (!location.IsIdentity()) {
                            p1.Transform(location.Transformation());
                            p2.Transform(location.Transformation());
                            p3.Transform(location.Transformation());
                        }
                        
                        // Create triangle
                        Mesh::Triangle meshTriangle;
                        meshTriangle.vertices[0] = Point3D(p1.X(), p1.Y(), p1.Z());
                        meshTriangle.vertices[1] = Point3D(p2.X(), p2.Y(), p2.Z());
                        meshTriangle.vertices[2] = Point3D(p3.X(), p3.Y(), p3.Z());
                        
                        // Calculate normal
                        Vector3D v1(p2.X() - p1.X(), p2.Y() - p1.Y(), p2.Z() - p1.Z());
                        Vector3D v2(p3.X() - p1.X(), p3.Y() - p1.Y(), p3.Z() - p1.Z());
                        
                        meshTriangle.normal = Vector3D(
                            v1.y * v2.z - v1.z * v2.y,
                            v1.z * v2.x - v1.x * v2.z,
                            v1.x * v2.y - v1.y * v2.x
                        ).normalized();
                        
                        mesh->addTriangle(meshTriangle);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        // Return empty mesh on error
        return std::make_unique<Mesh>();
    }
    
    return mesh;
}

std::vector<Point3D> OCCTPart::detectCylindricalFeatures() const {
    std::vector<Point3D> cylinderAxes;
    
    if (!m_shape) {
        return cylinderAxes;
    }
    
    try {
        const TopoDS_Shape* shape = static_cast<const TopoDS_Shape*>(m_shape);
        
        // Iterate through all faces to find cylindrical surfaces
        TopExp_Explorer faceExplorer(*shape, TopAbs_FACE);
        
        for (; faceExplorer.More(); faceExplorer.Next()) {
            const TopoDS_Face& face = TopoDS::Face(faceExplorer.Current());
            
            BRepAdaptor_Surface surface(face);
            
            if (surface.GetType() == GeomAbs_Cylinder) {
                // Found a cylindrical surface
                gp_Cylinder cylinder = surface.Cylinder();
                gp_Ax1 axis = cylinder.Axis();
                
                // Get cylinder parameters
                gp_Pnt location = axis.Location();
                gp_Dir direction = axis.Direction();
                
                // Store axis start and end points
                cylinderAxes.push_back(Point3D(location.X(), location.Y(), location.Z()));
                
                // Estimate cylinder length from face bounds
                double uMin, uMax, vMin, vMax;
                BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);
                
                double height = vMax - vMin;
                gp_Pnt endPoint = location.Translated(gp_Vec(direction.XYZ() * height));
                cylinderAxes.push_back(Point3D(endPoint.X(), endPoint.Y(), endPoint.Z()));
            }
        }
    } catch (const std::exception& e) {
        // Return empty list on error
        return std::vector<Point3D>();
    }
    
    return cylinderAxes;
}

std::optional<double> OCCTPart::getLargestCylinderDiameter() const {
    if (!m_shape) {
        return std::nullopt;
    }
    
    double largestDiameter = 0.0;
    bool foundCylinder = false;
    
    try {
        const TopoDS_Shape* shape = static_cast<const TopoDS_Shape*>(m_shape);
        
        // Iterate through all faces to find cylindrical surfaces
        TopExp_Explorer faceExplorer(*shape, TopAbs_FACE);
        
        for (; faceExplorer.More(); faceExplorer.Next()) {
            const TopoDS_Face& face = TopoDS::Face(faceExplorer.Current());
            
            BRepAdaptor_Surface surface(face);
            
            if (surface.GetType() == GeomAbs_Cylinder) {
                gp_Cylinder cylinder = surface.Cylinder();
                double radius = cylinder.Radius();
                double diameter = radius * 2.0;
                
                if (diameter > largestDiameter) {
                    largestDiameter = diameter;
                    foundCylinder = true;
                }
            }
        }
    } catch (const std::exception& e) {
        return std::nullopt;
    }
    
    return foundCylinder ? std::optional<double>(largestDiameter) : std::nullopt;
}

const TopoDS_Shape& OCCTPart::getOCCTShape() const {
    return *static_cast<const TopoDS_Shape*>(m_shape);
}

void OCCTPart::setOCCTShape(const TopoDS_Shape& shape) {
    if (m_shape) {
        delete static_cast<TopoDS_Shape*>(m_shape);
    }
    m_shape = new TopoDS_Shape(shape);
    m_boundingBoxComputed = false; // Invalidate cached bounding box
}

// WorkCoordinateSystem implementations
WorkCoordinateSystem::WorkCoordinateSystem(Type type) 
    : type_(type)
    , origin_(0, 0, 0)
    , xAxis_(1, 0, 0)
    , yAxis_(0, 1, 0) 
    , zAxis_(0, 0, 1)
    , toGlobal_(Matrix4x4::identity())
    , fromGlobal_(Matrix4x4::identity()) {
}

void WorkCoordinateSystem::setOrigin(const Point3D& origin) {
    origin_ = origin;
    updateTransformMatrices();
}

void WorkCoordinateSystem::setAxes(const Vector3D& xAxis, const Vector3D& yAxis, const Vector3D& zAxis) {
    xAxis_ = xAxis.normalized();
    yAxis_ = yAxis.normalized();
    zAxis_ = zAxis.normalized();
    updateTransformMatrices();
}

void WorkCoordinateSystem::setFromLatheMaterial(const Point3D& rawMaterialEnd, const Vector3D& spindleAxis) {
    // Set the origin at the end of the raw material (work coordinate zero)
    origin_ = rawMaterialEnd;
    
    // Z-axis is the spindle axis (direction of increasing Z in lathe coordinates)
    zAxis_ = spindleAxis.normalized();
    
    // X-axis is radial (perpendicular to spindle) - choose most appropriate direction
    // This represents the direction of increasing X (radius) in lathe coordinates
    if (std::abs(zAxis_.y) < 0.9) {
        // If Z-axis is not close to Y-axis, use Y direction for X-axis base
        Vector3D baseX(0, 1, 0);
        Vector3D temp = Vector3D(zAxis_.y * baseX.z - zAxis_.z * baseX.y,
                                zAxis_.z * baseX.x - zAxis_.x * baseX.z,
                                zAxis_.x * baseX.y - zAxis_.y * baseX.x);
        xAxis_ = Vector3D(temp.y * zAxis_.z - temp.z * zAxis_.y,
                         temp.z * zAxis_.x - temp.x * zAxis_.z,
                         temp.x * zAxis_.y - temp.y * zAxis_.x).normalized();
    } else {
        // If Z-axis is close to Y-axis, use X direction for X-axis base
        Vector3D baseX(1, 0, 0);
        Vector3D temp = Vector3D(zAxis_.y * baseX.z - zAxis_.z * baseX.y,
                                zAxis_.z * baseX.x - zAxis_.x * baseX.z,
                                zAxis_.x * baseX.y - zAxis_.y * baseX.x);
        xAxis_ = Vector3D(temp.y * zAxis_.z - temp.z * zAxis_.y,
                         temp.z * zAxis_.x - temp.x * zAxis_.z,
                         temp.x * zAxis_.y - temp.y * zAxis_.x).normalized();
    }
    
    // Y-axis completes the right-handed coordinate system (not used in 2D lathe ops)
    yAxis_ = Vector3D(zAxis_.y * xAxis_.z - zAxis_.z * xAxis_.y,
                      zAxis_.z * xAxis_.x - zAxis_.x * xAxis_.z,
                      zAxis_.x * xAxis_.y - zAxis_.y * xAxis_.x).normalized();
    
    updateTransformMatrices();
}

void WorkCoordinateSystem::updateTransformMatrices() {
    // Create transformation matrix from work coordinates to global coordinates
    // The columns of the matrix are the work coordinate axes expressed in global coordinates
    toGlobal_.data[0] = xAxis_.x;  toGlobal_.data[4] = yAxis_.x;  toGlobal_.data[8]  = zAxis_.x;  toGlobal_.data[12] = origin_.x;
    toGlobal_.data[1] = xAxis_.y;  toGlobal_.data[5] = yAxis_.y;  toGlobal_.data[9]  = zAxis_.y;  toGlobal_.data[13] = origin_.y;
    toGlobal_.data[2] = xAxis_.z;  toGlobal_.data[6] = yAxis_.z;  toGlobal_.data[10] = zAxis_.z;  toGlobal_.data[14] = origin_.z;
    toGlobal_.data[3] = 0.0;       toGlobal_.data[7] = 0.0;       toGlobal_.data[11] = 0.0;       toGlobal_.data[15] = 1.0;
    
    // The inverse transformation (global to work coordinates)
    fromGlobal_ = toGlobal_.inverse();
}

Point3D WorkCoordinateSystem::toGlobal(const Point3D& localPoint) const {
    return toGlobal_.transformPoint(localPoint);
}

Point3D WorkCoordinateSystem::fromGlobal(const Point3D& globalPoint) const {
    return fromGlobal_.transformPoint(globalPoint);
}

Vector3D WorkCoordinateSystem::toGlobal(const Vector3D& localVector) const {
    return toGlobal_.transformVector(localVector);
}

Vector3D WorkCoordinateSystem::fromGlobal(const Vector3D& globalVector) const {
    return fromGlobal_.transformVector(globalVector);
}

Point2D WorkCoordinateSystem::globalToLathe(const Point3D& globalPoint) const {
    // Convert global point to work coordinates first
    Point3D workPoint = fromGlobal(globalPoint);
    
    // In lathe coordinates: X = radius (distance from Z-axis), Z = axial position
    double radius = std::sqrt(workPoint.x * workPoint.x + workPoint.y * workPoint.y);
    double axial = workPoint.z;
    
    return Point2D(radius, axial);
}

Point3D WorkCoordinateSystem::latheToGlobal(const Point2D& lathePoint) const {
    // In lathe coordinates: X = radius, Z = axial
    // Convert to 3D work coordinates (radius in X direction, Z unchanged)
    Point3D workPoint(lathePoint.x, 0.0, lathePoint.z);
    
    // Transform to global coordinates
    return toGlobal(workPoint);
}

} // namespace Geometry
} // namespace IntuiCAM 