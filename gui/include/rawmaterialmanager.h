#ifndef RAWMATERIALMANAGER_H
#define RAWMATERIALMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>

// OpenCASCADE includes
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <gp_Ax1.hxx>

/**
 * @brief Manages raw material display and sizing
 * 
 * This class handles:
 * - Raw material cylinder creation and display
 * - Standard diameter matching
 * - Material properties and transparency
 * - Sizing calculations
 */
class RawMaterialManager : public QObject
{
    Q_OBJECT

public:
    explicit RawMaterialManager(QObject *parent = nullptr);
    ~RawMaterialManager();

    // Standard material diameters in mm (common turning stock sizes)
    static const QVector<double> STANDARD_DIAMETERS;

    /**
     * @brief Initialize with AIS context
     */
    void initialize(Handle(AIS_InteractiveContext) context);

    /**
     * @brief Create and display raw material cylinder
     */
    void displayRawMaterial(double diameter, double length, const gp_Ax1& axis);

    /**
     * @brief Find the next largest standard diameter for a given diameter
     */
    double getNextStandardDiameter(double diameter);

    /**
     * @brief Clear raw material display
     */
    void clearRawMaterial();

    /**
     * @brief Set transparency for raw material display
     */
    void setRawMaterialTransparency(double transparency = 0.7);

    /**
     * @brief Get current raw material shape
     */
    TopoDS_Shape getCurrentRawMaterial() const { return m_currentRawMaterial; }

    /**
     * @brief Check if raw material is currently displayed
     */
    bool isRawMaterialDisplayed() const { return !m_rawMaterialAIS.IsNull(); }

signals:
    /**
     * @brief Emitted when raw material is created and displayed
     */
    void rawMaterialCreated(double diameter, double length);

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& message);

private:
    Handle(AIS_InteractiveContext) m_context;
    
    // Raw material
    Handle(AIS_Shape) m_rawMaterialAIS;
    TopoDS_Shape m_currentRawMaterial;
    
    // Configuration
    double m_rawMaterialTransparency;
    
    /**
     * @brief Create a cylinder shape for raw material
     */
    TopoDS_Shape createCylinder(double diameter, double length, const gp_Ax1& axis);
    
    /**
     * @brief Set raw material visual properties
     */
    void setRawMaterialMaterial(Handle(AIS_Shape) rawMaterialAIS);
};

#endif // RAWMATERIALMANAGER_H 