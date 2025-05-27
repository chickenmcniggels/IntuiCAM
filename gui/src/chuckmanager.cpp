#include "chuckmanager.h"
#include "isteploader.h"

#include <QDebug>
#include <QFileInfo>

// OpenCASCADE includes
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Aspect_TypeOfFacingModel.hxx>
#include <Quantity_Color.hxx>

ChuckManager::ChuckManager(QObject *parent)
    : QObject(parent)
    , m_stepLoader(nullptr)
{
}

ChuckManager::~ChuckManager()
{
    // Don't delete m_stepLoader - it's owned by someone else
}

void ChuckManager::initialize(Handle(AIS_InteractiveContext) context, IStepLoader* stepLoader)
{
    m_context = context;
    m_stepLoader = stepLoader;
    qDebug() << "ChuckManager initialized with AIS context and STEP loader";
}

bool ChuckManager::loadChuck(const QString& chuckFilePath)
{
    if (m_context.IsNull()) {
        emit errorOccurred("AIS context not initialized");
        return false;
    }
    
    if (!m_stepLoader) {
        emit errorOccurred("STEP loader not initialized");
        return false;
    }

    QFileInfo fileInfo(chuckFilePath);
    if (!fileInfo.exists()) {
        emit errorOccurred("Chuck STEP file does not exist: " + chuckFilePath);
        return false;
    }

    // Load the chuck STEP file
    m_chuckShape = m_stepLoader->loadStepFile(chuckFilePath);
    
    if (m_chuckShape.IsNull() || !m_stepLoader->isValid()) {
        emit errorOccurred("Failed to load chuck STEP file: " + m_stepLoader->getLastError());
        return false;
    }

    // Create AIS shape for the chuck
    m_chuckAIS = new AIS_Shape(m_chuckShape);
    
    // Set chuck material properties
    setChuckMaterial(m_chuckAIS);
    
    // Display the chuck
    m_context->Display(m_chuckAIS, AIS_Shaded, 0, false);
    m_context->SetSelected(m_chuckAIS, false); // Ensure it's not selected
    
    emit chuckLoaded();
    qDebug() << "3-jaw chuck loaded and displayed successfully";
    return true;
}

void ChuckManager::clearChuck()
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Remove chuck
    if (!m_chuckAIS.IsNull()) {
        m_context->Remove(m_chuckAIS, false);
        m_chuckAIS.Nullify();
    }
    
    m_chuckShape = TopoDS_Shape();
    qDebug() << "Chuck cleared";
}

void ChuckManager::setChuckMaterial(Handle(AIS_Shape) chuckAIS)
{
    if (chuckAIS.IsNull()) {
        return;
    }
    
    // Set chuck material appearance (metallic gray)
    Graphic3d_MaterialAspect chuckMaterial(Graphic3d_NOM_STEEL);
    chuckMaterial.SetColor(Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB));
    chuckMaterial.SetAmbientColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    chuckMaterial.SetDiffuseColor(Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB));
    chuckMaterial.SetSpecularColor(Quantity_Color(0.9, 0.9, 0.9, Quantity_TOC_RGB));
    chuckMaterial.SetShininess(0.8);
    
    chuckAIS->SetMaterial(chuckMaterial);
} 