#include "rawmaterialmanager.h"

#include <QDebug>
#include <qmath.h>

// OpenCASCADE includes
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Ax2.hxx>
#include <AIS_DisplayMode.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Quantity_Color.hxx>

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