#include "chuckmanager.h"
#include <IntuiCAM/Geometry/IStepLoader.h>

#include <QDebug>
#include <QFileInfo>

// OpenCASCADE includes
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Aspect_TypeOfFacingModel.hxx>
#include <Quantity_Color.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Cylinder.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

ChuckManager::ChuckManager(QObject *parent)
    : QObject(parent)
    , m_stepLoader(nullptr)
    , m_centerlineDetected(false)
{
    // Initialize default chuck centerline axis (Z-axis through origin)
    m_chuckCenterlineAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
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
    m_chuckShape = m_stepLoader->loadStepFile(chuckFilePath.toStdString());
    
    if (m_chuckShape.IsNull() || !m_stepLoader->isValid()) {
        emit errorOccurred(QString("Failed to load chuck STEP file: %1")
                          .arg(QString::fromStdString(m_stepLoader->getLastError())));
        return false;
    }

    // Create AIS shape for the chuck
    m_chuckAIS = new AIS_Shape(m_chuckShape);
    
    // Set chuck material properties
    setChuckMaterial(m_chuckAIS);
    
    // Display the chuck
    m_context->Display(m_chuckAIS, AIS_Shaded, 0, false);
    m_context->SetSelected(m_chuckAIS, false); // Ensure it's not selected
    
    // Make chuck non-selectable and disable hover highlighting
    // Method 1: Deactivate all selection modes for the chuck to prevent selection
    m_context->Deactivate(m_chuckAIS);
    
    // Refresh viewer to ensure chuck becomes visible immediately
    m_context->UpdateCurrentViewer();
    
    // Method 2: Clear any existing selection on the chuck
    m_context->SetSelected(m_chuckAIS, false);
    
    qDebug() << "Chuck set as non-selectable";
    
    // Analyze chuck geometry to detect centerline
    analyzeChuckGeometry();
    
    // Verify that the chuck is properly configured as non-selectable
    isChuckNonSelectable();
    
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
        // Force viewer update to reflect removal
        m_context->UpdateCurrentViewer();
    }
    
    m_chuckShape = TopoDS_Shape();
    m_centerlineDetected = false;
    qDebug() << "Chuck cleared";
}

gp_Ax1 ChuckManager::getChuckCenterlineAxis() const
{
    return m_chuckCenterlineAxis;
}

bool ChuckManager::isChuckNonSelectable() const
{
    if (m_chuckAIS.IsNull() || m_context.IsNull()) {
        return false;
    }
    
    // Simplified verification: Just check if the chuck is displayed and not selected
    bool isDisplayed = m_context->IsDisplayed(m_chuckAIS);
    bool isSelected = m_context->IsSelected(m_chuckAIS);
    
    // Chuck should be displayed but not selected
    bool isNonSelectable = isDisplayed && !isSelected;
    
    if (isNonSelectable) {
        qDebug() << "Chuck verified as displayed and non-selected";
    } else {
        qDebug() << "Chuck selectability check - Displayed:" << isDisplayed 
                 << "Selected:" << isSelected;
    }
    
    return isNonSelectable;
}

bool ChuckManager::detectChuckCenterline()
{
    if (m_chuckShape.IsNull()) {
        qDebug() << "ChuckManager: No chuck loaded for centerline detection";
        return false;
    }
    
    analyzeChuckGeometry();
    return m_centerlineDetected;
}

void ChuckManager::setCustomChuckCenterline(const gp_Ax1& axis)
{
    m_chuckCenterlineAxis = axis;
    m_centerlineDetected = true;
    
    emit chuckCenterlineDetected(axis);
    qDebug() << "ChuckManager: Custom chuck centerline set";
}

void ChuckManager::analyzeChuckGeometry()
{
    if (m_chuckShape.IsNull()) {
        return;
    }
    
    try {
        // Look for cylindrical faces in the chuck to determine the main axis
        TopExp_Explorer faceExplorer(m_chuckShape, TopAbs_FACE);
        
        QVector<gp_Ax1> detectedAxes;
        QVector<double> cylinderRadii;
        
        for (; faceExplorer.More(); faceExplorer.Next()) {
            TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
            BRepAdaptor_Surface surface(face);
            
            if (surface.GetType() == GeomAbs_Cylinder) {
                gp_Cylinder cylinder = surface.Cylinder();
                gp_Ax1 axis = cylinder.Axis();
                double radius = cylinder.Radius();
                
                // Chuck cylinders are typically larger (main body) or smaller (jaw guides)
                // We're interested in the main body cylinder
                if (radius > 10.0 && radius < 200.0) { // Reasonable chuck size range
                    detectedAxes.append(axis);
                    cylinderRadii.append(radius);
                    
                    qDebug() << "ChuckManager: Detected cylindrical face with radius" << radius << "mm";
                }
            }
        }
        
        if (!detectedAxes.isEmpty()) {
            // Find the cylinder with the largest radius (likely the main chuck body)
            int maxRadiusIndex = 0;
            double maxRadius = cylinderRadii[0];
            
            for (int i = 1; i < cylinderRadii.size(); ++i) {
                if (cylinderRadii[i] > maxRadius) {
                    maxRadius = cylinderRadii[i];
                    maxRadiusIndex = i;
                }
            }
            
            // Use the axis of the largest cylinder as the chuck centerline
            m_chuckCenterlineAxis = detectedAxes[maxRadiusIndex];
            m_centerlineDetected = true;
            
            emit chuckCenterlineDetected(m_chuckCenterlineAxis);
            qDebug() << "ChuckManager: Chuck centerline detected from largest cylinder (radius:" << maxRadius << "mm)";
        } else {
            // Fallback: Use geometric center and Z-axis
            Bnd_Box bbox;
            BRepBndLib::Add(m_chuckShape, bbox);
            
            if (!bbox.IsVoid()) {
                double xmin, ymin, zmin, xmax, ymax, zmax;
                bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                
                gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
                m_chuckCenterlineAxis = gp_Ax1(center, gp_Dir(0, 0, 1));
                m_centerlineDetected = true;
                
                emit chuckCenterlineDetected(m_chuckCenterlineAxis);
                qDebug() << "ChuckManager: Chuck centerline estimated from bounding box center";
            } else {
                // Ultimate fallback: Origin with Z-axis
                m_chuckCenterlineAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
                m_centerlineDetected = true;
                
                emit chuckCenterlineDetected(m_chuckCenterlineAxis);
                qDebug() << "ChuckManager: Using default chuck centerline (origin, Z-axis)";
            }
        }
        
    } catch (const std::exception& e) {
        qDebug() << "ChuckManager: Error analyzing chuck geometry:" << e.what();
        
        // Fallback to default axis
        m_chuckCenterlineAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        m_centerlineDetected = true;
        emit chuckCenterlineDetected(m_chuckCenterlineAxis);
    }
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

void ChuckManager::redisplayChuck()
{
    if (m_context.IsNull() || m_chuckShape.IsNull()) {
        qDebug() << "ChuckManager: Cannot redisplay chuck - context null or chuck not loaded";
        return;
    }
    
    try {
        // Create new AIS shape for the chuck
        m_chuckAIS = new AIS_Shape(m_chuckShape);
        
        // Set chuck material properties
        setChuckMaterial(m_chuckAIS);
        
        // Display the chuck
        m_context->Display(m_chuckAIS, AIS_Shaded, 0, false);
        
        // Make chuck non-selectable and disable hover highlighting
        m_context->Deactivate(m_chuckAIS);
        m_context->SetSelected(m_chuckAIS, false);
        
        // Ensure the viewer refreshes after redisplay
        m_context->UpdateCurrentViewer();
        
        qDebug() << "ChuckManager: Chuck redisplayed successfully";
    }
    catch (const std::exception& e) {
        qDebug() << "ChuckManager: Error redisplaying chuck:" << e.what();
    }
}

void ChuckManager::setChuckVisible(bool visible)
{
    if (m_context.IsNull() || m_chuckAIS.IsNull()) {
        return;
    }

    if (visible) {
        if (!m_context->IsDisplayed(m_chuckAIS)) {
            m_context->Display(m_chuckAIS, Standard_False);
        }
    } else {
        m_context->Erase(m_chuckAIS, Standard_False);
    }

    m_context->UpdateCurrentViewer();
}

bool ChuckManager::isChuckVisible() const
{
    return !m_context.IsNull() && !m_chuckAIS.IsNull() && m_context->IsDisplayed(m_chuckAIS);
}
