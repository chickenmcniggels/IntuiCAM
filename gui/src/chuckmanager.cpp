#include "chuckmanager.h"
#include "steploader.h"

#include <QDebug>
#include <QFileInfo>

// OpenCASCADE includes
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepTools.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Ax2.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Aspect_TypeOfFacingModel.hxx>
#include <Quantity_Color.hxx>
#include <qmath.h>

// Standard material diameters in mm (ISO metric standard stock sizes)
const QVector<double> ChuckManager::STANDARD_DIAMETERS = {
    // Common turning stock diameters
    6.0, 8.0, 10.0, 12.0, 16.0, 20.0, 25.0, 30.0, 
    32.0, 40.0, 50.0, 60.0, 63.0, 80.0, 100.0, 110.0, 
    125.0, 140.0, 160.0, 180.0, 200.0, 220.0, 250.0, 
    280.0, 315.0, 355.0, 400.0, 450.0, 500.0
};

ChuckManager::ChuckManager(QObject *parent)
    : QObject(parent)
    , m_stepLoader(nullptr)
    , m_rawMaterialTransparency(0.7)
{
    m_stepLoader = new StepLoader();
}

ChuckManager::~ChuckManager()
{
    delete m_stepLoader;
}

void ChuckManager::initialize(Handle(AIS_InteractiveContext) context)
{
    m_context = context;
    qDebug() << "ChuckManager initialized with AIS context";
}

bool ChuckManager::loadChuck(const QString& chuckFilePath)
{
    if (m_context.IsNull()) {
        emit errorOccurred("AIS context not initialized");
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
    
    // Set chuck material appearance (metallic gray)
    Graphic3d_MaterialAspect chuckMaterial(Graphic3d_NOM_STEEL);
    chuckMaterial.SetColor(Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB));
    chuckMaterial.SetAmbientColor(Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB));
    chuckMaterial.SetDiffuseColor(Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB));
    chuckMaterial.SetSpecularColor(Quantity_Color(0.9, 0.9, 0.9, Quantity_TOC_RGB));
    chuckMaterial.SetShininess(0.8);
    
    m_chuckAIS->SetMaterial(chuckMaterial);
    
    // Display the chuck
    m_context->Display(m_chuckAIS, AIS_Shaded, 0, false);
    m_context->SetSelected(m_chuckAIS, false); // Ensure it's not selected
    
    qDebug() << "3-jaw chuck loaded and displayed successfully";
    return true;
}

bool ChuckManager::addWorkpiece(const TopoDS_Shape& workpiece)
{
    if (m_context.IsNull() || workpiece.IsNull()) {
        return false;
    }

    // Create AIS shape for the workpiece
    Handle(AIS_Shape) workpieceAIS = new AIS_Shape(workpiece);
    
    // Set workpiece material (aluminum-like)
    Graphic3d_MaterialAspect workpieceMaterial(Graphic3d_NOM_ALUMINIUM);
    workpieceMaterial.SetColor(Quantity_Color(0.8, 0.8, 0.9, Quantity_TOC_RGB));
    workpieceAIS->SetMaterial(workpieceMaterial);
    
    // Display the workpiece
    m_context->Display(workpieceAIS, AIS_Shaded, 0, false);
    m_workpieces.append(workpieceAIS);
    
    // Automatically detect cylinders and align if possible
    QVector<gp_Ax1> cylinders = detectCylinders(workpiece);
    if (!cylinders.isEmpty()) {
        // Use the first (largest) cylinder for alignment
        gp_Ax1 mainAxis = cylinders.first();
        alignWorkpieceWithChuck(workpiece, mainAxis);
    }
    
    return true;
}

QVector<gp_Ax1> ChuckManager::detectCylinders(const TopoDS_Shape& workpiece)
{
    QVector<gp_Ax1> cylinders;
    
    if (workpiece.IsNull()) {
        return cylinders;
    }
    
    // Analyze cylindrical faces
    analyzeCylindricalFaces(workpiece, cylinders);
    
    return cylinders;
}

void ChuckManager::analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders)
{
    TopExp_Explorer faceExplorer(shape, TopAbs_FACE);
    
    for (; faceExplorer.More(); faceExplorer.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExplorer.Current());
        BRepAdaptor_Surface surface(face);
        
        if (surface.GetType() == GeomAbs_Cylinder) {
            gp_Cylinder cylinder = surface.Cylinder();
            gp_Ax1 axis = cylinder.Axis();
            double radius = cylinder.Radius();
            double diameter = 2.0 * radius;
            
            // Only consider cylinders with reasonable diameters (> 5mm, < 500mm)
            if (diameter > 5.0 && diameter < 500.0) {
                cylinders.append(axis);
                emit cylinderDetected(diameter, 100.0, axis); // Default length estimate
                qDebug() << "Detected cylinder: diameter =" << diameter << "mm";
            }
        }
    }
}

double ChuckManager::getNextStandardDiameter(double diameter)
{
    for (double standardDiam : STANDARD_DIAMETERS) {
        if (standardDiam > diameter) {
            return standardDiam;
        }
    }
    
    // If larger than our largest standard, round up to next 50mm increment
    return qCeil(diameter / 50.0) * 50.0;
}

bool ChuckManager::alignWorkpieceWithChuck(const TopoDS_Shape& workpiece, const gp_Ax1& cylinderAxis)
{
    if (workpiece.IsNull()) {
        return false;
    }
    
    // Calculate workpiece bounding box to estimate required raw material length
    Bnd_Box bbox;
    BRepBndLib::Add(workpiece, bbox);
    
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    
    // Estimate required length (add 20% margin)
    double requiredLength = qMax(xmax - xmin, qMax(ymax - ymin, zmax - zmin)) * 1.2;
    
    // For demonstration, assume we detected a 25mm diameter cylinder
    double workpieceDiameter = 25.0; // This should come from cylinder detection
    double rawMaterialDiameter = getNextStandardDiameter(workpieceDiameter);
    
    // Create and display raw material
    displayRawMaterial(rawMaterialDiameter, requiredLength, cylinderAxis);
    
    emit workpieceAligned(rawMaterialDiameter, requiredLength);
    qDebug() << "Workpiece aligned - Raw material diameter:" << rawMaterialDiameter << "mm, Length:" << requiredLength << "mm";
    
    return true;
}

void ChuckManager::displayRawMaterial(double diameter, double length, const gp_Ax1& axis)
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Remove existing raw material if any
    if (!m_rawMaterialAIS.IsNull()) {
        m_context->Remove(m_rawMaterialAIS, false);
    }
    
    // Create raw material cylinder
    m_currentRawMaterial = createCylinder(diameter, length, axis);
    
    if (!m_currentRawMaterial.IsNull()) {
        m_rawMaterialAIS = new AIS_Shape(m_currentRawMaterial);
        
        // Set transparent material for raw material
        Graphic3d_MaterialAspect rawMaterial(Graphic3d_NOM_BRASS);
        rawMaterial.SetColor(Quantity_Color(0.8, 0.7, 0.3, Quantity_TOC_RGB));
        rawMaterial.SetTransparency(m_rawMaterialTransparency);
        
        m_rawMaterialAIS->SetMaterial(rawMaterial);
        m_rawMaterialAIS->SetTransparency(m_rawMaterialTransparency);
        
        // Display the raw material
        m_context->Display(m_rawMaterialAIS, AIS_Shaded, 0, false);
        
        qDebug() << "Raw material displayed - Diameter:" << diameter << "mm, Length:" << length << "mm";
    }
}

TopoDS_Shape ChuckManager::createCylinder(double diameter, double length, const gp_Ax1& axis)
{
    try {
        double radius = diameter / 2.0;
        
        // Create coordinate system for the target axis
        gp_Ax2 targetAx2(axis.Location(), axis.Direction());
        
        // Create cylinder directly with the target coordinate system
        BRepPrimAPI_MakeCylinder cylinderMaker(targetAx2, radius, length);
        TopoDS_Shape cylinder = cylinderMaker.Shape();
        
        return cylinder;
        
    } catch (const std::exception& e) {
        qDebug() << "Error creating cylinder:" << e.what();
        return TopoDS_Shape();
    }
}

void ChuckManager::clearWorkpieces()
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Remove all workpieces
    for (Handle(AIS_Shape) workpiece : m_workpieces) {
        if (!workpiece.IsNull()) {
            m_context->Remove(workpiece, false);
        }
    }
    m_workpieces.clear();
    
    // Remove raw material
    if (!m_rawMaterialAIS.IsNull()) {
        m_context->Remove(m_rawMaterialAIS, false);
        m_rawMaterialAIS.Nullify();
    }
    
    // Keep the chuck displayed
    qDebug() << "Workpieces and raw material cleared";
}

void ChuckManager::setRawMaterialTransparency(double transparency)
{
    m_rawMaterialTransparency = qBound(0.0, transparency, 1.0);
    
    if (!m_rawMaterialAIS.IsNull()) {
        m_rawMaterialAIS->SetTransparency(m_rawMaterialTransparency);
        m_context->Redisplay(m_rawMaterialAIS, false);
    }
} 