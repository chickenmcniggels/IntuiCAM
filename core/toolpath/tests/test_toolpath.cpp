#include <gtest/gtest.h>
#include <IntuiCAM/Toolpath/Types.h>
#include <IntuiCAM/Geometry/Types.h>

using namespace IntuiCAM::Toolpath;
using namespace IntuiCAM::Geometry;

// Test fixture providing a reusable Tool instance
class ToolpathCoreTest : public ::testing::Test {
protected:
    std::shared_ptr<Tool> tool;

    void SetUp() override {
        tool = std::make_shared<Tool>(Tool::Type::Turning, "TestTool");
    }
};

// -----------------------------------------------------------------------------
// Movement addition convenience helpers
// -----------------------------------------------------------------------------
TEST_F(ToolpathCoreTest, AddMovementsIncreaseCountAndPreserveOrder) {
    Toolpath tp("Path1", tool);
    EXPECT_EQ(tp.getMovementCount(), 0u);

    // Add rapid then linear
    tp.addRapidMove(Point3D(0, 0, 0));
    tp.addLinearMove(Point3D(10, 0, 0), 100.0);

    ASSERT_EQ(tp.getMovementCount(), 2u);

    const auto &moves = tp.getMovements();
    EXPECT_EQ(moves[0].type, MovementType::Rapid);
    EXPECT_EQ(moves[1].type, MovementType::Linear);
    EXPECT_DOUBLE_EQ(moves[1].feedRate, 100.0);
}

// -----------------------------------------------------------------------------
// Bounding-box computation
// -----------------------------------------------------------------------------
TEST_F(ToolpathCoreTest, BoundingBoxComputationIsCorrect) {
    Toolpath tp("BBoxTest", tool);
    tp.addRapidMove(Point3D(0, 0, 0));
    tp.addRapidMove(Point3D(10, 5, -2));

    const BoundingBox bbox = tp.getBoundingBox();

    EXPECT_EQ(bbox.min.x, 0);
    EXPECT_EQ(bbox.min.y, 0);
    EXPECT_EQ(bbox.min.z, -2);
    EXPECT_EQ(bbox.max.x, 10);
    EXPECT_EQ(bbox.max.y, 5);
    EXPECT_EQ(bbox.max.z, 0);
}

// -----------------------------------------------------------------------------
// Machining-time estimate – simple linear move case
// -----------------------------------------------------------------------------
TEST_F(ToolpathCoreTest, EstimateMachiningTimeSingleLinearMove) {
    Toolpath tp("TimeTest", tool);
    tp.addRapidMove(Point3D(0, 0, 0));
    tp.addLinearMove(Point3D(100, 0, 0), 100.0); // 100 mm at 100 mm/min ⇒ 1 min

    const double timeMin = tp.estimateMachiningTime();
    EXPECT_NEAR(timeMin, 1.0, 1e-6);
}

// -----------------------------------------------------------------------------
// Optimisation removes redundant consecutive moves at identical positions
// -----------------------------------------------------------------------------
TEST_F(ToolpathCoreTest, OptimizeToolpathRemovesRedundantMoves) {
    Toolpath tp("OptTest", tool);
    Point3D p0(0, 0, 0);
    Point3D p1(5, 0, 0);

    tp.addRapidMove(p0);
    tp.addRapidMove(p0);              // Redundant
    tp.addLinearMove(p1, 200.0);
    tp.addLinearMove(p1, 200.0);      // Redundant

    tp.optimizeToolpath();

    EXPECT_EQ(tp.getMovementCount(), 2u);
    EXPECT_EQ(tp.getMovements()[0].position.x, 0);
    EXPECT_EQ(tp.getMovements()[1].position.x, 5);
} 