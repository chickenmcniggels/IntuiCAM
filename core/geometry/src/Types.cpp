#include <IntuiCAM/Geometry/Types.h>
#include <cmath>
#include <algorithm>

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

} // namespace Geometry
} // namespace IntuiCAM 