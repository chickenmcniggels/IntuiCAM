#include "./../include/MainWindow.h"
#include "./../include/IntuiCAMViewerWidget.h"
#include <IntuiCAMCore/io/StepLoader.h>

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

using namespace IntuiCAM::GUI;

IntuiCAMMainWindow::IntuiCAMMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("IntuiCAM");
    resize(800, 600);

    m_viewerWidget = new IntuiCAMViewerWidget(this);
    setCentralWidget(m_viewerWidget);

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* importAct = fileMenu->addAction(tr("&Import"));
    connect(importAct, &QAction::triggered,
            this, &IntuiCAMMainWindow::onImportStepFile);
}

void IntuiCAMMainWindow::onImportStepFile() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Import STEP file"), QString(),
        tr("STEP Files (*.step *.stp)"));
    if (path.isEmpty())
        return;

    TopoDS_Shape shape;
    if (!IntuiCAM::Core::IO::StepLoader::loadStep(path.toStdString(), shape)
        || shape.IsNull())
    {
        QMessageBox::warning(this, tr("Import Failed"),
                             tr("Could not load the selected STEP file."));
        return;
    }

    m_viewerWidget->clear();
    m_viewerWidget->displayShape(shape);
}
