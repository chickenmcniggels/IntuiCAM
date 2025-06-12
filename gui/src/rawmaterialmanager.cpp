#include "rawmaterialmanager.h"

#include <QDebug>
#include <qmath.h>
#include <limits>
#include <algorithm>

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
#include <BRepBuilderAPI_Transform.hxx>

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
    , m_rawMaterialTransparency(0.6)
    , m_currentDiameter(0.0)
    , m_facingAllowance(10.0)
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
        
        // Make raw material non-selectable immediately after display
        makeRawMaterialNonSelectable();
        
        // Force immediate context update for real-time feedback
        m_context->UpdateCurrentViewer();
        
        // Store the current diameter
        m_currentDiameter = diameter;
        
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
        
        // Make raw material non-selectable immediately after display
        makeRawMaterialNonSelectable();
        
        // Force immediate context update for real-time feedback
        m_context->UpdateCurrentViewer();
        
        // Store the current diameter
        m_currentDiameter = diameter;
        
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
    
    if (!m_rawMaterialAIS.IsNull()) {
        m_context->Remove(m_rawMaterialAIS, false);
        m_rawMaterialAIS.Nullify();
        // Force immediate update after removing raw material for real-time feedback
        m_context->UpdateCurrentViewer();
    }
    
    // Reset current diameter
    m_currentDiameter = 0.0;
    
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

void RawMaterialManager::setRawMaterialVisible(bool visible)
{
    if (m_context.IsNull() || m_rawMaterialAIS.IsNull()) {
        return;
    }

    if (visible) {
        if (!m_context->IsDisplayed(m_rawMaterialAIS)) {
            m_context->Display(m_rawMaterialAIS, AIS_Shaded, 0, Standard_False);
        }
    } else {
        m_context->Erase(m_rawMaterialAIS, Standard_False);
    }

    m_context->UpdateCurrentViewer();
}

bool RawMaterialManager::isRawMaterialVisible() const
{
    return !m_context.IsNull() && !m_rawMaterialAIS.IsNull() && m_context->IsDisplayed(m_rawMaterialAIS);
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

void RawMaterialManager::makeRawMaterialNonSelectable()
{
    if (m_context.IsNull() || m_rawMaterialAIS.IsNull()) {
        return;
    }
    
    // Deactivate all selection modes for the raw material to prevent selection
    m_context->Deactivate(m_rawMaterialAIS);
    
    // Clear any existing selection on the raw material
    m_context->SetSelected(m_rawMaterialAIS, false);
    
    qDebug() << "Raw material set as non-selectable";
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
        gp_Dir axisDir = axis.Direction();
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
        
        // For lathe operations, ensure raw material:
        // 1. Always extends exactly 50mm in -Z direction (into chuck) from Z=0
        // 2. Includes extra stock to the right of the part for facing operations
        double chuckExtension = 50.0;     // Fixed 50mm extension into chuck
        double facingAllowance = m_facingAllowance;    // Extra stock for facing operations (right side)
        
        // Chuck face is at Z=0, calculate total length needed
        double chuckFacePosition = 0.0;   // Chuck face at Z=0
        double rawMaterialStart = chuckFacePosition - chuckExtension;  // Always at Z=-50
        double rawMaterialEnd = maxProjection + facingAllowance;
        
        // Ensure raw material extends far enough to cover the part
        rawMaterialEnd = std::max(rawMaterialEnd, chuckFacePosition + 20.0); // Minimum 20mm past chuck face
        
        double rawMaterialLength = rawMaterialEnd - rawMaterialStart;
        
        // Ensure minimum length of 70mm (50mm chuck + 20mm minimum working length)
        rawMaterialLength = std::max(rawMaterialLength, 70.0);
        
        qDebug() << "Calculated raw material length:" << rawMaterialLength << "mm for workpiece length:" << workpieceLength << "mm"
                 << "from chuck face at" << chuckFacePosition << "mm with chuck extension:" << chuckExtension << "mm and facing allowance:" << facingAllowance << "mm";
        
        return rawMaterialLength;
        
    } catch (const std::exception& e) {
        qDebug() << "Error calculating optimal length:" << e.what();
        return 100.0;
    }
}

double RawMaterialManager::calculateOptimalLengthWithTransform(const TopoDS_Shape& workpiece, const gp_Ax1& axis, const gp_Trsf& transform)
{
    if (workpiece.IsNull()) {
        return 100.0; // Default length
    }
    
    try {
        // Apply transformation to the workpiece for calculation
        TopoDS_Shape transformedWorkpiece = workpiece;
        if (!transform.Form() == gp_Identity) {
            BRepBuilderAPI_Transform transformer(workpiece, transform);
            transformedWorkpiece = transformer.Shape();
        }
        
        // Get bounding box of the transformed workpiece
        Bnd_Box bbox;
        BRepBndLib::Add(transformedWorkpiece, bbox);
        
        if (bbox.IsVoid()) {
            qDebug() << "Empty bounding box for transformed workpiece";
            return 100.0;
        }
        
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        // Calculate the bounds along the rotation axis
        gp_Dir axisDir = axis.Direction();
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
        
        // For lathe operations, ensure raw material:
        // 1. Always extends exactly 50mm in -Z direction (into chuck) from Z=0
        // 2. Includes extra stock to the right of the part for facing operations
        double chuckExtension = 50.0;     // Fixed 50mm extension into chuck
        double facingAllowance = m_facingAllowance;    // Extra stock for facing operations (right side)
        
        // Chuck face is at Z=0, calculate total length needed
        double chuckFacePosition = 0.0;   // Chuck face at Z=0
        double rawMaterialStart = chuckFacePosition - chuckExtension;  // Always at Z=-50
        double rawMaterialEnd = maxProjection + facingAllowance;
        
        // Ensure raw material extends far enough to cover the part
        rawMaterialEnd = std::max(rawMaterialEnd, chuckFacePosition + 20.0); // Minimum 20mm past chuck face
        
        double rawMaterialLength = rawMaterialEnd - rawMaterialStart;
        
        // Ensure minimum length of 70mm (50mm chuck + 20mm minimum working length)
        rawMaterialLength = std::max(rawMaterialLength, 70.0);
        
        qDebug() << "Calculated raw material length with transform:" << rawMaterialLength << "mm for transformed workpiece length:" << workpieceLength << "mm"
                 << "from chuck face at" << chuckFacePosition << "mm with chuck extension:" << chuckExtension << "mm and facing allowance:" << facingAllowance << "mm";
        
        return rawMaterialLength;
        
    } catch (const std::exception& e) {
        qDebug() << "Error calculating optimal length with transform:" << e.what();
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
        
        gp_Pnt cylinderStartPoint;
        if (!bbox.IsVoid()) {
            // Calculate the proper positioning along the axis
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            
            gp_Dir axisDir = axis.Direction();
            gp_Pnt axisLoc = axis.Location();
            
            // Project all bounding box corners onto the axis to find the true extent
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
            
            // For lathe operations, ensure raw material:
            // 1. Always extends exactly 50mm in -Z direction (into chuck) from Z=0
            // 2. Includes extra stock to the right of the part for facing operations
            double chuckExtension = 50.0;     // Fixed 50mm extension into chuck
            double facingAllowance = m_facingAllowance;    // Extra stock for facing operations (right side)
            
            // Chuck face is at Z=0, raw material starts at Z=-50
            double chuckFacePosition = 0.0;   // Chuck face at Z=0
            double rawMaterialStart = chuckFacePosition - chuckExtension;  // Always at Z=-50
            
            // Calculate end position: include full part + facing allowance (Z+ direction)
            double rawMaterialEnd = maxProjection + facingAllowance;
            
            // Ensure raw material extends far enough in Z+ to cover the part
            // If part is positioned away from chuck, raw material must extend further
            rawMaterialEnd = std::max(rawMaterialEnd, chuckFacePosition + 20.0); // Minimum 20mm past chuck face
            
            // Update the cylinder start point and length
            cylinderStartPoint = axisLoc.Translated(gp_Vec(axisDir) * rawMaterialStart);
            length = rawMaterialEnd - rawMaterialStart;
            
            qDebug() << "Raw material positioning:";
            qDebug() << "  Part extent:" << minProjection << "to" << maxProjection << "mm";
            qDebug() << "  Chuck face position:" << chuckFacePosition << "mm";
            qDebug() << "  Raw material:" << rawMaterialStart << "to" << rawMaterialEnd << "mm";
            qDebug() << "  Chuck extension:" << chuckExtension << "mm";
            qDebug() << "  Facing allowance:" << facingAllowance << "mm";
            qDebug() << "  Total length:" << length << "mm";
            
        } else {
            // Fallback to axis location if no valid bounds
            cylinderStartPoint = axis.Location().Translated(gp_Vec(axis.Direction()) * (-length / 2.0));
            qDebug() << "Using fallback positioning - no valid workpiece bounds";
        }
        
        // Create coordinate system for the cylinder starting at the calculated position
        gp_Ax2 cylinderAx2(cylinderStartPoint, axis.Direction());
        
        qDebug() << "Raw material cylinder creation:";
        qDebug() << "  Start point:" << cylinderStartPoint.X() << "," << cylinderStartPoint.Y() << "," << cylinderStartPoint.Z();
        qDebug() << "  Axis direction:" << axis.Direction().X() << "," << axis.Direction().Y() << "," << axis.Direction().Z();
        qDebug() << "  Radius:" << radius << "mm, Length:" << length << "mm";
        
        // Create cylinder starting from the calculated start point
        BRepPrimAPI_MakeCylinder cylinderMaker(cylinderAx2, radius, length);
        TopoDS_Shape cylinder = cylinderMaker.Shape();
        
        return cylinder;
        
    } catch (const std::exception& e) {
        qDebug() << "Error creating cylinder for workpiece:" << e.what();
        emit errorOccurred(QString("Error creating cylinder for workpiece: %1").arg(e.what()));
        return TopoDS_Shape();
    }
}

TopoDS_Shape RawMaterialManager::createCylinderForWorkpieceWithTransform(double diameter, double length, const gp_Ax1& axis, const TopoDS_Shape& workpiece, const gp_Trsf& transform)
{
    try {
        double radius = diameter / 2.0;
        
        // Apply transformation to the workpiece for bounding box calculation
        TopoDS_Shape transformedWorkpiece = workpiece;
        if (!transform.Form() == gp_Identity) {
            BRepBuilderAPI_Transform transformer(workpiece, transform);
            transformedWorkpiece = transformer.Shape();
        }
        
        // Get transformed workpiece bounding box to determine proper positioning
        Bnd_Box bbox;
        BRepBndLib::Add(transformedWorkpiece, bbox);
        
        gp_Pnt cylinderStartPoint;
        if (!bbox.IsVoid()) {
            // Calculate the proper positioning along the axis
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            
            gp_Dir axisDir = axis.Direction();
            gp_Pnt axisLoc = axis.Location();
            
            // Project all bounding box corners onto the axis to find the true extent
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
            
            // For lathe operations, ensure raw material:
            // 1. Always extends exactly 50mm in -Z direction (into chuck) from Z=0
            // 2. Includes extra stock to the right of the part for facing operations
            double chuckExtension = 50.0;     // Fixed 50mm extension into chuck
            double facingAllowance = m_facingAllowance;    // Extra stock for facing operations (right side)
            
            // Chuck face is at Z=0, raw material starts at Z=-50
            double chuckFacePosition = 0.0;   // Chuck face at Z=0
            double rawMaterialStart = chuckFacePosition - chuckExtension;  // Always at Z=-50
            
            // Calculate end position: include full part + facing allowance (Z+ direction)
            double rawMaterialEnd = maxProjection + facingAllowance;
            
            // Ensure raw material extends far enough in Z+ to cover the part
            // If part is positioned away from chuck, raw material must extend further
            rawMaterialEnd = std::max(rawMaterialEnd, chuckFacePosition + 20.0); // Minimum 20mm past chuck face
            
            // Update the cylinder start point and length
            cylinderStartPoint = axisLoc.Translated(gp_Vec(axisDir) * rawMaterialStart);
            length = rawMaterialEnd - rawMaterialStart;
            
            qDebug() << "Raw material positioning with transform:";
            qDebug() << "  Transformed part extent:" << minProjection << "to" << maxProjection << "mm";
            qDebug() << "  Chuck face position:" << chuckFacePosition << "mm";
            qDebug() << "  Raw material:" << rawMaterialStart << "to" << rawMaterialEnd << "mm";
            qDebug() << "  Chuck extension:" << chuckExtension << "mm";
            qDebug() << "  Facing allowance:" << facingAllowance << "mm";
            qDebug() << "  Total length:" << length << "mm";
            
        } else {
            // Fallback to axis location if no valid bounds
            cylinderStartPoint = axis.Location().Translated(gp_Vec(axis.Direction()) * (-length / 2.0));
            qDebug() << "Using fallback positioning - no valid transformed workpiece bounds";
        }
        
        // Create coordinate system for the cylinder starting at the calculated position
        gp_Ax2 cylinderAx2(cylinderStartPoint, axis.Direction());
        
        qDebug() << "Raw material cylinder creation:";
        qDebug() << "  Start point:" << cylinderStartPoint.X() << "," << cylinderStartPoint.Y() << "," << cylinderStartPoint.Z();
        qDebug() << "  Axis direction:" << axis.Direction().X() << "," << axis.Direction().Y() << "," << axis.Direction().Z();
        qDebug() << "  Radius:" << radius << "mm, Length:" << length << "mm";
        
        // Create cylinder starting from the calculated start point
        BRepPrimAPI_MakeCylinder cylinderMaker(cylinderAx2, radius, length);
        TopoDS_Shape cylinder = cylinderMaker.Shape();
        
        return cylinder;
        
    } catch (const std::exception& e) {
        qDebug() << "Error creating cylinder for workpiece with transform:" << e.what();
        emit errorOccurred(QString("Error creating cylinder for workpiece with transform: %1").arg(e.what()));
        return TopoDS_Shape();
    }
}

void RawMaterialManager::displayRawMaterialForWorkpieceWithTransform(double diameter, const TopoDS_Shape& workpiece, const gp_Ax1& axis, const gp_Trsf& transform)
{
    if (m_context.IsNull()) {
        emit errorOccurred("AIS context not initialized");
        return;
    }
    
    if (workpiece.IsNull()) {
        emit errorOccurred("Invalid workpiece provided");
        return;
    }
    
    // Calculate workpiece bounds along the specified axis with transform
    double length = calculateOptimalLengthWithTransform(workpiece, axis, transform);
    
    // Remove existing raw material if any
    clearRawMaterial();
    
    // Create raw material cylinder that encompasses the entire transformed workpiece
    m_currentRawMaterial = createCylinderForWorkpieceWithTransform(diameter, length, axis, workpiece, transform);
    
    if (!m_currentRawMaterial.IsNull()) {
        m_rawMaterialAIS = new AIS_Shape(m_currentRawMaterial);
        
        // Set raw material visual properties
        setRawMaterialMaterial(m_rawMaterialAIS);
        
        // Display the raw material
        m_context->Display(m_rawMaterialAIS, AIS_Shaded, 0, false);
        
        // Make raw material non-selectable immediately after display
        makeRawMaterialNonSelectable();
        
        // Force immediate context update for real-time feedback
        m_context->UpdateCurrentViewer();
        
        // Store the current diameter
        m_currentDiameter = diameter;
        
        emit rawMaterialCreated(diameter, length);
        qDebug() << "Raw material displayed for transformed workpiece - Diameter:" << diameter << "mm, Length:" << length << "mm";
    } else {
        emit errorOccurred("Failed to create raw material cylinder for transformed workpiece");
    }
}

void RawMaterialManager::setFacingAllowance(double allowance)
{
    m_facingAllowance = std::max(0.0, allowance);
    qDebug() << "RawMaterialManager: Facing allowance set to" << m_facingAllowance << "mm";
} 