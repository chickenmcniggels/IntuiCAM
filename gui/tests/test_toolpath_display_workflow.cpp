#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QtTest/QtTest>

#include "workspcecontroller.h"
#include "toolpathgenerationcontroller.h"
#include "toolpathtimelinewidget.h"
#include "toolpathmanager.h"
#include "workpiecemanager.h"
#include "rawmaterialmanager.h"
#include "toolmanager.h"

// Mock class to intercept calls to the real ToolpathManager
class MockToolpathManager : public ToolpathManager
{
    Q_OBJECT
public:
    explicit MockToolpathManager(QObject *parent = nullptr) : ToolpathManager(parent) {}

    bool displayToolpath(const IntuiCAM::Toolpath::Toolpath& toolpath, const QString& name) override
    {
        wasDisplayToolpathCalled = true;
        lastToolpathName = name;
        return true; // Simulate success
    }

    bool wasDisplayToolpathCalled = false;
    QString lastToolpathName;
};

class TestToolpathDisplayWorkflow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testAddToolpathEndToEnd();

private:
    WorkspaceController* m_workspaceController;
    ToolpathGenerationController* m_generationController;
    ToolpathTimelineWidget* m_timelineWidget;
    MockToolpathManager* m_mockToolpathManager;
};

void TestToolpathDisplayWorkflow::initTestCase()
{
    // Set up the application objects
    m_workspaceController = new WorkspaceController();
    m_mockToolpathManager = new MockToolpathManager();
    // Inject the mock manager into the controller
    m_generationController = new ToolpathGenerationController(m_mockToolpathManager);
    m_timelineWidget = new ToolpathTimelineWidget();
    
    m_workspaceController->initialize(nullptr); // Pass null context for non-GUI test
    m_generationController->setWorkspaceController(m_workspaceController);
    m_generationController->connectTimelineWidget(m_timelineWidget);

    // Load a dummy workpiece and raw material to allow adding toolpaths
    m_workspaceController->getWorkpieceManager()->setWorkpieceShape(TopoDS_Shape());
    m_workspaceController->getRawMaterialManager()->createRawMaterial(100, 200);
    // Create a dummy tool
    m_workspaceController->getToolManager()->addTool(std::make_shared<IntuiCAM::Toolpath::Tool>("Test Tool", IntuiCAM::Toolpath::Tool::Type::Facing));
}

void TestToolpathDisplayWorkflow::cleanupTestCase()
{
    delete m_timelineWidget;
    delete m_generationController;
    delete m_workspaceController;
    // The mock manager is now a child of the test case, so we don't delete it
    // as it's owned by the generation controller which is deleted.
}

void TestToolpathDisplayWorkflow::testAddToolpathEndToEnd()
{
    // Pre-condition check
    QVERIFY(m_mockToolpathManager->wasDisplayToolpathCalled == false);

    // This test still can't click the dialog. So we can't test the whole flow.
    // However, with the refactoring, the core logic is in `handleToolpathAddRequest`.
    // And that function calls `generateAndStoreToolpath`.
    // The test for `handleToolpathAddRequest` is more of an integration test.
    
    // Instead of calling the handler, which pops a dialog, we will call the
    // `generateAndStoreToolpath` method directly. This is more of a unit test
    // for the generation and display logic, which is what we want to verify.
    
    int testIndex = 0;
    QString testOpName = "Facing 1";
    QString testOpType = "Facing";
    auto tool = m_workspaceController->getToolManager()->getTool("Test Tool");
    QVariantMap params; // empty params for this test

    // Call the function that contains the core logic we want to test
    m_generationController->generateAndStoreToolpath(testIndex, testOpName, testOpType, tool, params);

    // It runs async and posts back to the main thread, so we need to wait a bit
    // for the QMetaObject::invokeMethod to run.
    QTest::qWait(1000); 

    // Assert that the display method on our mock manager was called
    QVERIFY(m_mockToolpathManager->wasDisplayToolpathCalled);
    QCOMPARE(m_mockToolpathManager->lastToolpathName, testOpName);
}

QTEST_MAIN(TestToolpathDisplayWorkflow)
#include "test_toolpath_display_workflow.moc" 