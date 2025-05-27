#include "workpiecemanager.h"

#include <QDebug>

// OpenCASCADE includes
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Ax1.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Quantity_Color.hxx>

WorkpieceManager::WorkpieceManager(QObject *parent)
    : QObject(parent)
    , m_detectedDiameter(0.0)
{
    // Initialize main cylinder axis to default (Z-axis)
    m_mainCylinderAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
}

WorkpieceManager::~WorkpieceManager()
{
}

void WorkpieceManager::initialize(Handle(AIS_InteractiveContext) context)
{
    m_context = context;
    qDebug() << "WorkpieceManager initialized with AIS context";
}

bool WorkpieceManager::addWorkpiece(const TopoDS_Shape& workpiece)
{
    if (m_context.IsNull() || workpiece.IsNull()) {
        emit errorOccurred("Invalid context or workpiece");
        return false;
    }

    // Create AIS shape for the workpiece
    Handle(AIS_Shape) workpieceAIS = new AIS_Shape(workpiece);
    
    // Set workpiece material properties
    setWorkpieceMaterial(workpieceAIS);
    
    // Display the workpiece
    m_context->Display(workpieceAIS, AIS_Shaded, 0, false);
    m_workpieces.append(workpieceAIS);
    
    // Detect cylinders in the workpiece
    QVector<gp_Ax1> cylinders = detectCylinders(workpiece);
    if (!cylinders.isEmpty()) {
        // Store the main (first/largest) cylinder
        m_mainCylinderAxis = cylinders.first();
    }
    
    qDebug() << "Workpiece added to scene with" << cylinders.size() << "cylinders detected";
    return true;
}

QVector<gp_Ax1> WorkpieceManager::detectCylinders(const TopoDS_Shape& workpiece)
{
    QVector<gp_Ax1> cylinders;
    
    if (workpiece.IsNull()) {
        return cylinders;
    }
    
    // Reset detected diameter
    m_detectedDiameter = 0.0;
    
    // Analyze cylindrical faces
    analyzeCylindricalFaces(workpiece, cylinders);
    
    return cylinders;
}

void WorkpieceManager::analyzeCylindricalFaces(const TopoDS_Shape& shape, QVector<gp_Ax1>& cylinders)
{
    TopExp_Explorer faceExplorer(shape, TopAbs_FACE);
    
    double largestDiameter = 0.0;
    
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
                
                // Track the largest diameter for main cylinder identification
                if (diameter > largestDiameter) {
                    largestDiameter = diameter;
                    m_detectedDiameter = diameter;
                }
                
                emit cylinderDetected(diameter, 100.0, axis); // Default length estimate
                qDebug() << "Detected cylinder: diameter =" << diameter << "mm";
            }
        }
    }
}

void WorkpieceManager::clearWorkpieces()
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
    
    // Reset analysis results
    m_detectedDiameter = 0.0;
    m_mainCylinderAxis = gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
    
    qDebug() << "All workpieces cleared";
}

void WorkpieceManager::setWorkpieceMaterial(Handle(AIS_Shape) workpieceAIS)
{
    if (workpieceAIS.IsNull()) {
        return;
    }
    
    // Set workpiece material (aluminum-like)
    Graphic3d_MaterialAspect workpieceMaterial(Graphic3d_NOM_ALUMINIUM);
    workpieceMaterial.SetColor(Quantity_Color(0.8, 0.8, 0.9, Quantity_TOC_RGB));
    workpieceAIS->SetMaterial(workpieceMaterial);
} 