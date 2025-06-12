#include <QtTest/QtTest>
#include <QSignalSpy>
#include <toolpathmanager.h>
#include <IntuiCAM/Toolpath/Types.h>

// Mock OpenCASCADE context for testing
#include <AIS_InteractiveContext.hxx>
#include <V3d_Viewer.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>

class ToolpathManagerTest : public QObject {
    Q_OBJECT
private:
    // Create mock AIS context for testing
    Handle(AIS_InteractiveContext) createMockContext() {
        Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
        Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection);
        
        Handle(V3d_Viewer) viewer = new V3d_Viewer(graphicDriver);
        viewer->SetDefaultLights();
        viewer->SetLightOn();
        
        Handle(AIS_InteractiveContext) context = new AIS_InteractiveContext(viewer);
        return context;
    }

private slots:
    void testDisplayToolpath();
    void testClearAllToolpaths();
    void testToolpathVisibility();
};

void ToolpathManagerTest::testDisplayToolpath()
{
    // Create toolpath manager
    ToolpathManager manager;
    
    // Initialize with mock context
    manager.initialize(createMockContext());
    
    // Create a simple toolpath
    auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(
        IntuiCAM::Toolpath::Tool::Type::Turning, "TestTool");
    
    IntuiCAM::Toolpath::Toolpath toolpath("TestPath", tool);
    
    // Add some movements
    toolpath.addRapidMove(IntuiCAM::Geometry::Point3D(0, 0, 0));
    toolpath.addLinearMove(IntuiCAM::Geometry::Point3D(10, 0, 0), 100.0);
    
    // Test signal emission
    QSignalSpy spy(&manager, &ToolpathManager::toolpathDisplayed);
    
    // Display the toolpath
    bool result = manager.displayToolpath(toolpath, "TestPath");
    
    // Verify result and signal
    QVERIFY(result);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("TestPath"));
}

void ToolpathManagerTest::testClearAllToolpaths()
{
    // Create toolpath manager
    ToolpathManager manager;
    
    // Initialize with mock context
    manager.initialize(createMockContext());
    
    // Create a simple toolpath
    auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(
        IntuiCAM::Toolpath::Tool::Type::Turning, "TestTool");
    
    IntuiCAM::Toolpath::Toolpath toolpath("TestPath", tool);
    toolpath.addRapidMove(IntuiCAM::Geometry::Point3D(0, 0, 0));
    
    // Display the toolpath
    manager.displayToolpath(toolpath, "TestPath");
    
    // Test signal emission for clear
    QSignalSpy spy(&manager, &ToolpathManager::allToolpathsCleared);
    
    // Clear all toolpaths
    manager.clearAllToolpaths();
    
    // Verify signal
    QCOMPARE(spy.count(), 1);
}

void ToolpathManagerTest::testToolpathVisibility()
{
    // Create toolpath manager
    ToolpathManager manager;
    
    // Initialize with mock context
    manager.initialize(createMockContext());
    
    // Create a simple toolpath
    auto tool = std::make_shared<IntuiCAM::Toolpath::Tool>(
        IntuiCAM::Toolpath::Tool::Type::Turning, "TestTool");
    
    IntuiCAM::Toolpath::Toolpath toolpath("TestPath", tool);
    toolpath.addRapidMove(IntuiCAM::Geometry::Point3D(0, 0, 0));
    
    // Display the toolpath
    manager.displayToolpath(toolpath, "TestPath");
    
    // Toggle visibility (this just tests that the method executes without crashing)
    manager.setToolpathVisible("TestPath", false);
    
    // Toggle back
    manager.setToolpathVisible("TestPath", true);
    
    // No assertion here since we can't easily check actual visibility without UI
    // This is primarily a smoke test
}

QTEST_MAIN(ToolpathManagerTest)
#include "test_toolpathmanager.moc" 