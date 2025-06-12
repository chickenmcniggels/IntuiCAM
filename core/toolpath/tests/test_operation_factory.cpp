#include <gtest/gtest.h>
#include <IntuiCAM/Toolpath/Operations.h>
#include <IntuiCAM/Toolpath/FacingOperation.h>
#include <IntuiCAM/Toolpath/RoughingOperation.h>
#include <IntuiCAM/Toolpath/FinishingOperation.h>
#include <IntuiCAM/Toolpath/PartingOperation.h>
#include <IntuiCAM/Toolpath/ThreadingOperation.h>
#include <IntuiCAM/Toolpath/GroovingOperation.h>
#include <typeinfo>

using namespace IntuiCAM::Toolpath;

TEST(OperationFactory, CreatesCorrectDerivedTypes) {
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "FactoryTool");

    struct TestCase {
        Operation::Type type;
        const char* name;
        const std::type_info& info;
    } cases[] = {
        { Operation::Type::Facing, "Face", typeid(FacingOperation) },
        { Operation::Type::Roughing, "Rough", typeid(RoughingOperation) },
        { Operation::Type::Finishing, "Finish", typeid(FinishingOperation) },
        { Operation::Type::Parting, "Part", typeid(PartingOperation) },
        { Operation::Type::Threading, "Thread", typeid(ThreadingOperation) },
        { Operation::Type::Grooving, "Groove", typeid(GroovingOperation) },
    };

    for (const auto& tc : cases) {
        std::unique_ptr<Operation> op = Operation::createOperation(tc.type, tc.name, tool);
        ASSERT_NE(op, nullptr) << "Factory returned null for " << tc.name;
        EXPECT_EQ(op->getType(), tc.type);
        EXPECT_EQ(typeid(*op), tc.info);
    }
}

TEST(RoughingOperationValidation, DetectsInvalidParameters) {
    auto tool = std::make_shared<Tool>(Tool::Type::Turning, "ValidateTool");
    RoughingOperation op("Rough", tool);

    // Default parameters should be valid
    EXPECT_TRUE(op.validate());

    // Invalid case: start diameter smaller than end diameter
    RoughingOperation::Parameters p = op.getParameters();
    p.startDiameter = 10; // smaller than endDiameter (20)
    op.setParameters(p);
    EXPECT_FALSE(op.validate());
} 