#include "rawmaterialmanager.h"

#include <QDebug>
#include <qmath.h>
#include <limits>

// OpenCASCADE includes
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Ax2.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Quantity_Color.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

// Standard material diameters in mm (ISO metric standard stock sizes)
const QVector<double> RawMaterialManager::STANDARD_DIAMETERS = {
    // Common turning stock diameters
    6.0, 8.0, 10.0, 12.0, 16.0, 20.0, 25.0, 30.0, 
    32.0, 40.0, 50.0, 60.0, 63.0, 80.0, 100.0, 110.0, 
    125.0, 140.0, 160.0, 180.0, 200.0, 220.0, 250.0, 
    280.0, 315.0, 355.0, 400.0, 450.0, 500.0
};

RawMaterialManager::RawMaterialManager(QObject *parent)
    : QObject(parent)
    , m_rawMaterialTransparency(0.7)
{
}

RawMaterialManager::~RawMaterialManager()
{
}

void RawMaterialManager::initialize(Handle(AIS_InteractiveContext) context)
{
    m_context = context;
    qDebug() << "RawMaterialManager initialized with AIS context";
}

void RawMaterialManager::displayRawMaterial(double diameter, double length, const gp_Ax1& axis)
{
    if (m_context.IsNull()) {
        emit errorOccurred("AIS context not initialized");
        return;
    }
    
    // Remove existing raw material if any
    clearRawMaterial();
    
    // Create raw material cylinder
    m_currentRawMaterial = createCylinder(diameter, length, axis);
    
    if (!m_currentRawMaterial.IsNull()) {
        m_rawMaterialAIS = new AIS_Shape(m_currentRawMaterial);
        
        // Set raw material visual properties
        setRawMaterialMaterial(m_rawMaterialAIS);
        
        // Display the raw material
        m_context->Display(m_rawMaterialAIS, AIS_Shaded, 0, false);
        
        emit rawMaterialCreated(diameter, length);
        qDebug() << "Raw material displayed - Diameter:" << diameter << "mm, Length:" << length << "mm";
    } else {
        emit errorOccurred("Failed to create raw material cylinder");
    }
}

void RawMaterialManager::displayRawMaterialForWorkpiece(double diameter, const TopoDS_Shape& workpiece, const gp_Ax1& axis)
{
    if (m_context.IsNull()) {
        emit errorOccurred("AIS context not initialized");
        return;
    }
    
    if (workpiece.IsNull()) {
        emit errorOccurred("Invalid workpiece provided");
        return;
    }
    
    // Calculate workpiece bounds along the specified axis
    double length = calculateOptimalLength(workpiece, axis);
    
    // Remove existing raw material if any
    clearRawMaterial();
    
    // Create raw material cylinder that encompasses the entire workpiece
    m_currentRawMaterial = createCylinderForWorkpiece(diameter, length, axis, workpiece);
    
    if (!m_currentRawMaterial.IsNull()) {
        m_rawMaterialAIS = new AIS_Shape(m_currentRawMaterial);
        
        // Set raw material visual properties
        setRawMaterialMaterial(m_rawMaterialAIS);
        
        // Display the raw material
        m_context->Display(m_rawMaterialAIS, AIS_Shaded, 0, false);
        
        emit rawMaterialCreated(diameter, length);
        qDebug() << "Raw material displayed for workpiece - Diameter:" << diameter << "mm, Length:" << length << "mm";
    } else {
        emit errorOccurred("Failed to create raw material cylinder for workpiece");
    }
}

double RawMaterialManager::getNextStandardDiameter(double diameter)
{
    for (double standardDiam : STANDARD_DIAMETERS) {
        if (standardDiam > diameter) {
            return standardDiam;
        }
    }
    
    // If larger than our largest standard, round up to next 50mm increment
    return qCeil(diameter / 50.0) * 50.0;
}

void RawMaterialManager::clearRawMaterial()
{
    if (m_context.IsNull()) {
        return;
    }
    
    // Remove raw material
    if (!m_rawMaterialAIS.IsNull()) {
        m_context->Remove(m_rawMaterialAIS, false);
        m_rawMaterialAIS.Nullify();
    }
    
    m_currentRawMaterial = TopoDS_Shape();
    qDebug() << "Raw material cleared";
}

void RawMaterialManager::setRawMaterialTransparency(double transparency)
{
    m_rawMaterialTransparency = qBound(0.0, transparency, 1.0);
    
    if (!m_rawMaterialAIS.IsNull()) {
        m_rawMaterialAIS->SetTransparency(m_rawMaterialTransparency);
        m_context->Redisplay(m_rawMaterialAIS, false);
    }
}

TopoDS_Shape RawMaterialManager::createCylinder(double diameter, double length, const gp_Ax1& axis)
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
        emit errorOccurred(QString("Error creating cylinder: %1").arg(e.what()));
        return TopoDS_Shape();
    }
}

void RawMaterialManager::setRawMaterialMaterial(Handle(AIS_Shape) rawMaterialAIS)
{
    if (rawMaterialAIS.IsNull()) {
        return;
    }
    
    // Set transparent material for raw material
    Graphic3d_MaterialAspect rawMaterial(Graphic3d_NOM_BRASS);
    rawMaterial.SetColor(Quantity_Color(0.8, 0.7, 0.3, Quantity_TOC_RGB));
    rawMaterial.SetTransparency(m_rawMaterialTransparency);
    
    rawMaterialAIS->SetMaterial(rawMaterial);
    rawMaterialAIS->SetTransparency(m_rawMaterialTransparency);
}

void RawMaterialManager::setCustomDiameter(double diameter, const TopoDS_Shape& workpiece, const gp_Ax1& axis)
{
    if (diameter <= 0.0) {
        emit errorOccurred("Invalid diameter specified");
        return;
    }
    
    if (workpiece.IsNull()) {
        // Use default length if no workpiece provided
        displayRawMaterial(diameter, 100.0, axis);
    } else {
        displayRawMaterialForWorkpiece(diameter, workpiece, axis);
    }
}

double RawMaterialManager::calculateOptimalLength(const TopoDS_Shape& workpiece, const gp_Ax1& axis)
{
    if (workpiece.IsNull()) {
        return 100.0; // Default length
    }
    
    try {
        // Get bounding box of the workpiece
        Bnd_Box bbox;
        BRepBndLib::Add(workpiece, bbox);
        
        if (bbox.IsVoid()) {
            qDebug() << "Empty bounding box for workpiece";
            return 100.0;
        }
        
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        // Calculate the bounds along the rotation axis
        gp_Vec axisDir = axis.Direction();
        gp_Pnt axisLoc = axis.Location();
        
        // Project bounding box corners onto the axis to find extent
        double minProjection = std::numeric_limits<double>::max();
        double maxProjection = std::numeric_limits<double>::lowest();
        
        // Check all 8 corners of the bounding box
        gp_Pnt corners[8] = {
            gp_Pnt(xmin, ymin, zmin), gp_Pnt(xmax, ymin, zmin),
            gp_Pnt(xmin, ymax, zmin), gp_Pnt(xmax, ymax, zmin),
            gp_Pnt(xmin, ymin, zmax), gp_Pnt(xmax, ymin, zmax),
            gp_Pnt(xmin, ymax, zmax), gp_Pnt(xmax, ymax, zmax)
        };
        
        for (int i = 0; i < 8; i++) {
            gp_Vec toCorner(axisLoc, corners[i]);
            double projection = toCorner.Dot(axisDir);
            minProjection = std::min(minProjection, projection);
            maxProjection = std::max(maxProjection, projection);
        }
        
        double workpieceLength = maxProjection - minProjection;
        
        // Add 20% extra length (10% on each end) for machining allowance
        double rawMaterialLength = workpieceLength * 1.2;
        
        // Ensure minimum length of 10mm
        rawMaterialLength = std::max(rawMaterialLength, 10.0);
        
        qDebug() << "Calculated raw material length:" << rawMaterialLength << "mm for workpiece length:" << workpieceLength << "mm";
        
        return rawMaterialLength;
        
    } catch (const std::exception& e) {
        qDebug() << "Error calculating optimal length:" << e.what();
        return 100.0;
    }
}

TopoDS_Shape RawMaterialManager::createCylinderForWorkpiece(double diameter, double length, const gp_Ax1& axis, const TopoDS_Shape& workpiece)
{
    try {
        double radius = diameter / 2.0;
        
        // Get workpiece bounding box to determine proper positioning
        Bnd_Box bbox;
        BRepBndLib::Add(workpiece, bbox);
        
        gp_Pnt cylinderCenter;
        if (!bbox.IsVoid()) {
            // Calculate the center of the raw material cylinder along the axis
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            
            gp_Vec axisDir = axis.Direction();
            gp_Pnt axisLoc = axis.Location();
            
            // Project bounding box center onto the axis
            gp_Pnt bboxCenter((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
            gp_Vec toBboxCenter(axisLoc, bboxCenter);
            double projection = toBboxCenter.Dot(axisDir);
            
            // Position cylinder center so that it encompasses the workpiece
            cylinderCenter = axisLoc.Translated(axisDir * (projection - length / 2.0));
        } else {
            // Fallback to axis location if no valid bounds
            cylinderCenter = axis.Location().Translated(axis.Direction() * (-length / 2.0));
        }
        
        // Create coordinate system for the cylinder
        gp_Ax2 cylinderAx2(cylinderCenter, axis.Direction());
        
        // Create cylinder
        BRepPrimAPI_MakeCylinder cylinderMaker(cylinderAx2, radius, length);
        TopoDS_Shape cylinder = cylinderMaker.Shape();
        
        return cylinder;
        
    } catch (const std::exception& e) {
        qDebug() << "Error creating cylinder for workpiece:" << e.what();
        emit errorOccurred(QString("Error creating cylinder for workpiece: %1").arg(e.what()));
        return TopoDS_Shape();
    }
} 