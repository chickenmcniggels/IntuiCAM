#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTest>
#include <toolpathtimelinewidget.h>

class PlusButtonWorkflowTest : public QObject {
    Q_OBJECT
private slots:
        // Ensure the add-toolpath menu emits the expected signal when the '+' button is used
        void testPlusButtonEmitsAddToolpathRequested();
};

void PlusButtonWorkflowTest::testPlusButtonEmitsAddToolpathRequested()
{
    // GIVEN a fresh timeline widget with the + button enabled
    ToolpathTimelineWidget widget;
    widget.setAddButtonEnabled(true);
    widget.show();

    // Spy on the addToolpathRequested signal
    QSignalSpy addSpy(&widget, &ToolpathTimelineWidget::addToolpathRequested);
    QVERIFY(addSpy.isValid());

    // Locate the '+' button – it has the objectName set in the implementation
    QPushButton *plusBtn = widget.findChild<QPushButton*>("addToolpathButton");
    QVERIFY2(plusBtn, "Unable to locate '+' button");

    // WHEN the '+' button is clicked and the very first item in the popup menu is triggered
    // 1) Simulate the mouse click which opens the popup menu
    QTest::mouseClick(plusBtn, Qt::LeftButton);

    // 2) Grab the popup menu that was created inside the widget
    QMenu *popupMenu = widget.findChild<QMenu*>();
    QVERIFY2(popupMenu, "Popup \"Add Toolpath\" menu not found");

    // Trigger the first action (typically "Facing")
    QAction *firstAction = popupMenu->actions().first();
    QVERIFY2(firstAction, "No action found in popup menu");
    firstAction->trigger();

    // THEN the timeline widget must emit exactly one addToolpathRequested signal
    QVERIFY2(addSpy.wait(250), "addToolpathRequested signal not emitted in time");
    QCOMPARE(addSpy.count(), 1);
    const QList<QVariant> arguments = addSpy.takeFirst();

    // Verify the payload is non-empty (e.g. "Facing", "Roughing"…)
    QVERIFY(!arguments.at(0).toString().isEmpty());
}

QTEST_MAIN(PlusButtonWorkflowTest)
#include "test_plusbuttonworkflow.moc" 