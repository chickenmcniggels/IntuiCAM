#include <gtest/gtest.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>

using namespace IntuiCAM;

// -----------------------------------------------------------------------------
// Minimal mock part implementation (axis-aligned cylinder) ---------------------
// -----------------------------------------------------------------------------
class MockCylinderPart : public Geometry::Part {
public:
    double getVolume() const override { return 100000.0; }
    double getSurfaceArea() const override { return 20000.0; }

    Geometry::BoundingBox getBoundingBox() const override {
        return {Geometry::Point3D(-25.0, -25.0, -50.0),
                Geometry::Point3D( 25.0,  25.0,  50.0)};
    }

    std::unique_ptr<Geometry::GeometricEntity> clone() const override {
        return std::make_unique<MockCylinderPart>(*this);
    }

    std::unique_ptr<Geometry::Mesh> generateMesh(double /*tolerance*/) const override {
        return std::make_unique<Geometry::Mesh>();
    }

    std::vector<Geometry::Point3D> detectCylindricalFeatures() const override {
        return {Geometry::Point3D(0,0,-50), Geometry::Point3D(0,0,50)};
    }

    std::optional<double> getLargestCylinderDiameter() const override {
        return 50.0; // 50 mm diameter
    }
};

// Shared fixture providing a reusable tool and mock part
class OperationGenerationTest : public ::testing::Test {
protected:
    std::shared_ptr<Toolpath::Tool> tool_;
    MockCylinderPart part_;

    void SetUp() override {
        tool_ = std::make_shared<Toolpath::Tool>(Toolpath::Tool::Type::Turning, "TestTurningTool");
    }
};

// -----------------------------------------------------------------------------
// Facing operation -------------------------------------------------------------
// -----------------------------------------------------------------------------
TEST_F(OperationGenerationTest, FacingOperationGeneratesMovements) {
    auto facingOp = std::make_unique<Toolpath::FacingOperation>("FacingTest", tool_);

    // Default parameters are sufficient for this high-level test
    auto toolpath = facingOp->generateToolpath(part_);

    ASSERT_NE(toolpath, nullptr);
    EXPECT_GT(toolpath->getMovementCount(), 0u) << "Facing operation produced an empty toolpath";
}

// -----------------------------------------------------------------------------
// Roughing operation -----------------------------------------------------------
// -----------------------------------------------------------------------------
TEST_F(OperationGenerationTest, RoughingOperationGeneratesMovements) {
    auto roughOp = std::make_unique<Toolpath::RoughingOperation>("RoughingTest", tool_);

    Toolpath::RoughingOperation::Parameters params;
    params.startDiameter = 50.0;
    params.endDiameter   = 20.0;
    params.startZ        = 0.0;
    params.endZ          = -50.0;
    params.depthOfCut    = 2.0;
    roughOp->setParameters(params);

    auto toolpath = roughOp->generateToolpath(part_);

    ASSERT_NE(toolpath, nullptr);
    EXPECT_GT(toolpath->getMovementCount(), 0u) << "Roughing operation produced an empty toolpath";
} 