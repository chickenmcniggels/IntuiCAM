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
    // Widget now creates standard operations on construction
    QCOMPARE(m_widget->getToolpathCount(), 4);

    int idx = m_widget->addToolpath("Extra_001", "Contouring", "Tool1");
    QCOMPARE(idx, 4);
    QCOMPARE(m_widget->getToolpathCount(), 5);
    QCOMPARE(m_widget->getToolpathName(idx), QString("Extra_001"));

    // Update name and verify
    m_widget->updateToolpath(idx, "Extra_002", "Contouring", "Tool1");
    QCOMPARE(m_widget->getToolpathName(idx), QString("Extra_002"));

    // Remove and verify count
    m_widget->removeToolpath(idx);
    QCOMPARE(m_widget->getToolpathCount(), 4);

    // Clear check
    m_widget->addToolpath("Threading_001", "Threading", "Tool2");
    m_widget->addToolpath("Chamfering_001", "Chamfering", "Tool3");
    QCOMPARE(m_widget->getToolpathCount(), 6);
    m_widget->clearToolpaths();
    // Clearing resets to zero custom toolpaths but keeps defaults
    QCOMPARE(m_widget->getToolpathCount(), 0);
}

void ToolpathTimelineWidgetTest::testActiveToolpathSignal()
{
    m_widget->addToolpath("Contouring_001", "Contouring", "Tool1");
    m_widget->addToolpath("Threading_001", "Threading", "Tool2");

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
    QAction action("Contouring", m_widget);
    QMetaObject::invokeMethod(m_widget, "onOperationTypeSelected", Qt::DirectConnection,
                              Q_ARG(QAction*, &action));

    QTest::qWait(1); // Allow event loop to process

    QVERIFY(spy.isValid());
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), "Contouring");
}

QTEST_MAIN(ToolpathTimelineWidgetTest)
#include "test_toolpathtimelinewidget.moc" 