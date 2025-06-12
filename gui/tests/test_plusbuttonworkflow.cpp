#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTest>
#include <toolpathtimelinewidget.h>

class PlusButtonWorkflowTest : public QObject {
    Q_OBJECT
private slots:
        // Verify that the timeline no longer exposes an add-toolpath '+' button
        void testPlusButtonRemoved();
};

void PlusButtonWorkflowTest::testPlusButtonRemoved()
{
    // GIVEN a fresh timeline widget
    ToolpathTimelineWidget widget;
    widget.show();

    // THEN no child button with the old object name should exist
    QPushButton *plusBtn = widget.findChild<QPushButton*>("addToolpathButton");
    QVERIFY2(!plusBtn, "Unexpected '+' button found in timeline");
}

QTEST_MAIN(PlusButtonWorkflowTest)
#include "test_plusbuttonworkflow.moc" 