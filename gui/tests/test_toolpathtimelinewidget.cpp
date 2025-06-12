#include <QtTest/QtTest>
#include <QSignalSpy>
#include <toolpathtimelinewidget.h>

// The ToolpathTimelineWidget is in the global namespace, not in IntuiCAM::GUI
// using namespace IntuiCAM::GUI;

class ToolpathTimelineWidgetTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testAddRemoveUpdateClear();
    void testActiveToolpathSignal();
    void testAddToolpathSignalEmission();

private:
    ToolpathTimelineWidget* m_widget;
};

void ToolpathTimelineWidgetTest::initTestCase()
{
    m_widget = new ToolpathTimelineWidget();
}

void ToolpathTimelineWidgetTest::cleanupTestCase()
{
    delete m_widget;
}

void ToolpathTimelineWidgetTest::testAddRemoveUpdateClear()
{
    QCOMPARE(m_widget->getToolpathCount(), 0);

    int idx = m_widget->addToolpath("Facing_001", "Facing", "Tool1");
    QCOMPARE(idx, 0);
    QCOMPARE(m_widget->getToolpathCount(), 1);
    QCOMPARE(m_widget->getToolpathName(idx), QString("Facing_001"));

    // Update name and verify
    m_widget->updateToolpath(idx, "Facing_002", "Facing", "Tool1");
    QCOMPARE(m_widget->getToolpathName(idx), QString("Facing_002"));

    // Remove and verify count
    m_widget->removeToolpath(idx);
    QCOMPARE(m_widget->getToolpathCount(), 0);

    // Clear check
    m_widget->addToolpath("Roughing_001", "Roughing", "Tool2");
    m_widget->addToolpath("Finish_001", "Finishing", "Tool3");
    QCOMPARE(m_widget->getToolpathCount(), 2);
    m_widget->clearToolpaths();
    QCOMPARE(m_widget->getToolpathCount(), 0);
}

void ToolpathTimelineWidgetTest::testActiveToolpathSignal()
{
    m_widget->addToolpath("Facing_001", "Facing", "Tool1");
    m_widget->addToolpath("Roughing_001", "Roughing", "Tool2");

    QSignalSpy spy(m_widget, &ToolpathTimelineWidget::toolpathSelected);

    m_widget->setActiveToolpath(1);
    
    QTest::qWait(1); // Allow event loop to process

    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), 1);
}

void ToolpathTimelineWidgetTest::testAddToolpathSignalEmission()
{
    QSignalSpy spy(m_widget, &ToolpathTimelineWidget::addToolpathRequested);
    
    // The test environment can't easily simulate a menu click.
    // Instead, we invoke the slot that the menu action would trigger.
    QAction action("Facing", m_widget);
    QMetaObject::invokeMethod(m_widget, "onOperationTypeSelected", Qt::DirectConnection,
                              Q_ARG(QAction*, &action));

    QTest::qWait(1); // Allow event loop to process

    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), "Facing");
}

QTEST_MAIN(ToolpathTimelineWidgetTest)
#include "test_toolpathtimelinewidget.moc" 