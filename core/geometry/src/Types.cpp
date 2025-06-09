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
    
    // Create triangulation of the shape
    BRepMesh_IncrementalMesh incrementalMesh(*static_cast<TopoDS_Shape*>(m_shape), tolerance);
    incrementalMesh.Perform();
    
    // Iterate through all faces
    for (TopExp_Explorer faceExplorer(*static_cast<TopoDS_Shape*>(m_shape), TopAbs_FACE); 
         faceExplorer.More(); faceExplorer.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
        TopLoc_Location loc;
        
        // Get triangulation for this face
        Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);
        
        if (!triangulation.IsNull()) {
            // Newer versions of OpenCASCADE use a different method for accessing nodes
            int nbTriangles = triangulation->NbTriangles();
            int nbNodes = triangulation->NbNodes();
            
            // Iterate through all triangles
            for (int i = 1; i <= nbTriangles; i++) {
                // Get triangle vertices indices (1-based in OpenCASCADE)
                int n1, n2, n3;
                triangulation->Triangle(i).Get(n1, n2, n3);
                
                // Get node coordinates (1-based in OpenCASCADE)
                gp_Pnt p1 = triangulation->Node(n1).Transformed(loc);
                gp_Pnt p2 = triangulation->Node(n2).Transformed(loc);
                gp_Pnt p3 = triangulation->Node(n3).Transformed(loc);
                
                // Create triangle
                Mesh::Triangle triangle;
                triangle.vertices[0] = Point3D(p1.X(), p1.Y(), p1.Z());
                triangle.vertices[1] = Point3D(p2.X(), p2.Y(), p2.Z());
                triangle.vertices[2] = Point3D(p3.X(), p3.Y(), p3.Z());
                
                // Calculate normal (simplified)
                Vector3D v1(triangle.vertices[1].x - triangle.vertices[0].x,
                           triangle.vertices[1].y - triangle.vertices[0].y,
                           triangle.vertices[1].z - triangle.vertices[0].z);
                           
                Vector3D v2(triangle.vertices[2].x - triangle.vertices[0].x,
                           triangle.vertices[2].y - triangle.vertices[0].y,
                           triangle.vertices[2].z - triangle.vertices[0].z);
                           
                // Cross product for normal
                Vector3D normal;
                normal.x = v1.y * v2.z - v1.z * v2.y;
                normal.y = v1.z * v2.x - v1.x * v2.z;
                normal.z = v1.x * v2.y - v1.y * v2.x;
                
                // Normalize
                double length = std::sqrt(normal.x * normal.x + 
                                        normal.y * normal.y + 
                                        normal.z * normal.z);
                                        
                if (length > 0.0) {
                    normal.x /= length;
                    normal.y /= length;
                    normal.z /= length;
                    
                    // Reverse normal if needed
                    if (face.Orientation() == TopAbs_REVERSED) {
                        normal.x = -normal.x;
                        normal.y = -normal.y;
                        normal.z = -normal.z;
                    }
                }
                
                triangle.normal = normal;
                
                // Add triangle to mesh
                mesh->addTriangle(triangle);
            }
        }
    }
    
    return mesh;
}

std::vector<Point3D> OCCTPart::detectCylindricalFeatures() const {
    std::vector<Point3D> features;
    
    if (!m_shape) {
        return features;
    }
    
    try {
        // Explore all faces
        for (TopExp_Explorer faceExplorer(*static_cast<TopoDS_Shape*>(m_shape), TopAbs_FACE); 
             faceExplorer.More(); faceExplorer.Next()) {
            
            TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
            BRepAdaptor_Surface surface(face);
            
            // Check if the face is a cylinder
            if (surface.GetType() == GeomAbs_Cylinder) {
                // Get cylinder data
                gp_Cylinder cylinder = surface.Cylinder();
                gp_Ax1 axis = cylinder.Axis();
                gp_Pnt center = axis.Location();
                gp_Dir direction = axis.Direction();
                double radius = cylinder.Radius();
                
                // Add the center point
                features.push_back(Point3D(center.X(), center.Y(), center.Z()));
                
                // Add a point along the axis to indicate direction
                features.push_back(Point3D(
                    center.X() + direction.X() * radius,
                    center.Y() + direction.Y() * radius,
                    center.Z() + direction.Z() * radius
                ));
            }
        }
    } catch (const std::exception&) {
        // Handle exceptions silently
    }
    
    return features;
}

std::optional<double> OCCTPart::getLargestCylinderDiameter() const {
    if (!m_shape) {
        return std::nullopt;
    }
    
    try {
        double maxDiameter = 0.0;
        bool found = false;
        
        // Explore all faces
        for (TopExp_Explorer faceExplorer(*static_cast<TopoDS_Shape*>(m_shape), TopAbs_FACE); 
             faceExplorer.More(); faceExplorer.Next()) {
            
            TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
            BRepAdaptor_Surface surface(face);
            
            // Check if the face is a cylinder
            if (surface.GetType() == GeomAbs_Cylinder) {
                // Get cylinder radius
                double diameter = surface.Cylinder().Radius() * 2.0;
                
                // Update max diameter
                if (diameter > maxDiameter || !found) {
                    maxDiameter = diameter;
                    found = true;
                }
            }
        }
        
        if (found) {
            return maxDiameter;
        }
    } catch (const std::exception&) {
        // Handle exceptions silently
    }
    
    return std::nullopt;
}

const TopoDS_Shape& OCCTPart::getOCCTShape() const {
    if (!m_shape) {
        throw std::runtime_error("OCCTPart: Null shape accessed");
    }
    return *static_cast<TopoDS_Shape*>(m_shape);
}

void OCCTPart::setOCCTShape(const TopoDS_Shape& shape) {
    // Delete old shape if it exists
    if (m_shape) {
        delete static_cast<TopoDS_Shape*>(m_shape);
    }
    
    // Copy new shape
    m_shape = new TopoDS_Shape(shape);
    
    // Reset bounding box
    m_boundingBoxComputed = false;
}

} // namespace Geometry
} // namespace IntuiCAM 