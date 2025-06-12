#include <QtTest/QtTest>
#include <QSignalSpy>

#include <toolpathgenerationcontroller.h>
#include <toolpathtimelinewidget.h>
#include <workspacecontroller.h>

// -----------------------------------------------------------------------------
// Very light-weight mock workspace controller providing nullptr managers so that
// generateAndDisplayToolpath falls back to safe defaults (guard logic is now in
// place).
// -----------------------------------------------------------------------------
class SafeMockWorkspaceController : public WorkspaceController {
public:
    SafeMockWorkspaceController() : WorkspaceController(nullptr) {}

    // Return nullptr managers – controller now handles these safely.
    WorkpieceManager* getWorkpieceManager()   const override { return nullptr; }
    RawMaterialManager* getRawMaterialManager() const override { return nullptr; }
};

class ToolpathAdditionTest : public QObject {
    Q_OBJECT
private slots:
    void testControllerEmitsToolpathAddedAndTimelineUpdates();
};

void ToolpathAdditionTest::testControllerEmitsToolpathAddedAndTimelineUpdates()
{
    // Create UI components
    QWidget parentWidget; // dummy parent to own child widgets
    ToolpathTimelineWidget timeline(&parentWidget);

    // Controller
    IntuiCAM::GUI::ToolpathGenerationController controller;

    // Workspace mock – no real OCC context/managers
    auto workspace = std::make_shared<SafeMockWorkspaceController>();
    controller.setWorkspaceController(workspace.get());

    // Hook up timeline
    controller.connectTimelineWidget(&timeline);

    // Spy on toolpathAdded
    QSignalSpy addedSpy(&controller, &IntuiCAM::GUI::ToolpathGenerationController::toolpathAdded);

    // Manually invoke generation – use default tool inside controller helper
    auto tool = controller.createDefaultTool("Facing");

    QVERIFY(controller.generateAndDisplayToolpath("Facing_Test", "Facing", tool));

    // The controller should have emitted the signal at least once
    QVERIFY(addedSpy.wait(100)); // wait up to 100 ms
    QCOMPARE(addedSpy.count(), 1);

    // Timeline should now contain exactly one tile with the matching name
    QCOMPARE(timeline.getToolpathCount(), 1);
    QCOMPARE(timeline.getToolpathName(0), QString("Facing_Test"));
}

QTEST_MAIN(ToolpathAdditionTest)
#include "test_toolpath_addition.moc" 