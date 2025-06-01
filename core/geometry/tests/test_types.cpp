#include <gtest/gtest.h>
#include <IntuiCAM/Geometry/Types.h>

using namespace IntuiCAM::Geometry;

// Test suite for basic geometric types
class GeometryTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup code
        origin = Point3D(0, 0, 0);
        unitX = Vector3D(1, 0, 0);
        unitY = Vector3D(0, 1, 0);
        unitZ = Vector3D(0, 0, 1);
    }
    
    Point3D origin;
    Vector3D unitX, unitY, unitZ;
};

// Point3D tests
TEST_F(GeometryTypesTest, Point3DConstruction) {
    Point3D p1;
    EXPECT_EQ(p1.x, 0.0);
    EXPECT_EQ(p1.y, 0.0);
    EXPECT_EQ(p1.z, 0.0);
    
    Point3D p2(1.5, 2.5, 3.5);
    EXPECT_EQ(p2.x, 1.5);
    EXPECT_EQ(p2.y, 2.5);
    EXPECT_EQ(p2.z, 3.5);
}

// Vector3D tests
TEST_F(GeometryTypesTest, Vector3DMagnitude) {
    EXPECT_NEAR(unitX.magnitude(), 1.0, 1e-6);
    EXPECT_NEAR(unitY.magnitude(), 1.0, 1e-6);
    EXPECT_NEAR(unitZ.magnitude(), 1.0, 1e-6);
    
    Vector3D v(3, 4, 0);
    EXPECT_NEAR(v.magnitude(), 5.0, 1e-6);
}

TEST_F(GeometryTypesTest, Vector3DNormalization) {
    Vector3D v(3, 4, 0);
    Vector3D normalized = v.normalized();
    
    EXPECT_NEAR(normalized.magnitude(), 1.0, 1e-6);
    EXPECT_NEAR(normalized.x, 0.6, 1e-6);
    EXPECT_NEAR(normalized.y, 0.8, 1e-6);
    EXPECT_NEAR(normalized.z, 0.0, 1e-6);
}

// BoundingBox tests
TEST_F(GeometryTypesTest, BoundingBoxConstruction) {
    Point3D min(0, 0, 0);
    Point3D max(10, 20, 30);
    BoundingBox bbox(min, max);
    
    EXPECT_EQ(bbox.min.x, 0);
    EXPECT_EQ(bbox.min.y, 0);
    EXPECT_EQ(bbox.min.z, 0);
    EXPECT_EQ(bbox.max.x, 10);
    EXPECT_EQ(bbox.max.y, 20);
    EXPECT_EQ(bbox.max.z, 30);
}

TEST_F(GeometryTypesTest, BoundingBoxContains) {
    BoundingBox bbox(Point3D(0, 0, 0), Point3D(10, 10, 10));
    
    EXPECT_TRUE(bbox.contains(Point3D(5, 5, 5)));
    EXPECT_TRUE(bbox.contains(Point3D(0, 0, 0)));
    EXPECT_TRUE(bbox.contains(Point3D(10, 10, 10)));
    EXPECT_FALSE(bbox.contains(Point3D(-1, 5, 5)));
    EXPECT_FALSE(bbox.contains(Point3D(5, 11, 5)));
}

TEST_F(GeometryTypesTest, BoundingBoxIntersection) {
    BoundingBox bbox1(Point3D(0, 0, 0), Point3D(10, 10, 10));
    BoundingBox bbox2(Point3D(5, 5, 5), Point3D(15, 15, 15));
    BoundingBox bbox3(Point3D(20, 20, 20), Point3D(30, 30, 30));
    
    EXPECT_TRUE(bbox1.intersects(bbox2));
    EXPECT_TRUE(bbox2.intersects(bbox1));
    EXPECT_FALSE(bbox1.intersects(bbox3));
    EXPECT_FALSE(bbox3.intersects(bbox1));
}

TEST_F(GeometryTypesTest, BoundingBoxSize) {
    BoundingBox bbox(Point3D(0, 0, 0), Point3D(10, 20, 30));
    Vector3D size = bbox.size();
    
    EXPECT_EQ(size.x, 10);
    EXPECT_EQ(size.y, 20);
    EXPECT_EQ(size.z, 30);
}

TEST_F(GeometryTypesTest, BoundingBoxCenter) {
    BoundingBox bbox(Point3D(0, 0, 0), Point3D(10, 20, 30));
    Point3D center = bbox.center();
    
    EXPECT_EQ(center.x, 5);
    EXPECT_EQ(center.y, 10);
    EXPECT_EQ(center.z, 15);
}

// Matrix4x4 tests
TEST_F(GeometryTypesTest, Matrix4x4Identity) {
    Matrix4x4 identity = Matrix4x4::identity();
    
    // Check diagonal elements
    EXPECT_EQ(identity.data[0], 1.0);   // [0,0]
    EXPECT_EQ(identity.data[5], 1.0);   // [1,1]
    EXPECT_EQ(identity.data[10], 1.0);  // [2,2]
    EXPECT_EQ(identity.data[15], 1.0);  // [3,3]
    
    // Check some off-diagonal elements
    EXPECT_EQ(identity.data[1], 0.0);   // [0,1]
    EXPECT_EQ(identity.data[4], 0.0);   // [1,0]
}

TEST_F(GeometryTypesTest, Matrix4x4Translation) {
    Vector3D translation(5, 10, 15);
    Matrix4x4 transform = Matrix4x4::translation(translation);
    
    // Check translation components
    EXPECT_EQ(transform.data[12], 5.0);   // [0,3]
    EXPECT_EQ(transform.data[13], 10.0);  // [1,3]
    EXPECT_EQ(transform.data[14], 15.0);  // [2,3]
    
    // Check that it's still identity in rotation part
    EXPECT_EQ(transform.data[0], 1.0);    // [0,0]
    EXPECT_EQ(transform.data[5], 1.0);    // [1,1]
    EXPECT_EQ(transform.data[10], 1.0);   // [2,2]
}

// Mesh tests
TEST_F(GeometryTypesTest, MeshTriangleAddition) {
    Mesh mesh;
    
    Mesh::Triangle triangle;
    triangle.vertices[0] = Point3D(0, 0, 0);
    triangle.vertices[1] = Point3D(1, 0, 0);
    triangle.vertices[2] = Point3D(0, 1, 0);
    triangle.normal = Vector3D(0, 0, 1);
    
    mesh.addTriangle(triangle);
    
    EXPECT_EQ(mesh.getTriangleCount(), 1);
}

TEST_F(GeometryTypesTest, MeshBoundingBox) {
    Mesh mesh;
    
    // Add a triangle
    Mesh::Triangle triangle;
    triangle.vertices[0] = Point3D(0, 0, 0);
    triangle.vertices[1] = Point3D(10, 0, 0);
    triangle.vertices[2] = Point3D(5, 10, 5);
    triangle.normal = Vector3D(0, 0, 1);
    mesh.addTriangle(triangle);
    
    BoundingBox bbox = mesh.getBoundingBox();
    
    EXPECT_EQ(bbox.min.x, 0);
    EXPECT_EQ(bbox.min.y, 0);
    EXPECT_EQ(bbox.min.z, 0);
    EXPECT_EQ(bbox.max.x, 10);
    EXPECT_EQ(bbox.max.y, 10);
    EXPECT_EQ(bbox.max.z, 5);
} 