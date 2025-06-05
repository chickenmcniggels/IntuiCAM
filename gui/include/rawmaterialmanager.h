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
#include <gp_Trsf.hxx>

/**
 * @brief Manages raw material display and sizing
 * 
 * This class handles:
 * - Raw material cylinder creation and display
 * - Standard diameter matching
 * - Material properties and transparency
 * - Sizing calculations with precise positioning requirements:
 *   * Always extends exactly 50mm in -Z direction (into chuck)
 *   * Always includes 10mm extra stock to the right for facing operations
 *   * Recalculates automatically when workpiece position, diameter, or orientation changes
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
     * @brief Create and display raw material cylinder sized for a specific workpiece
     * @param diameter Raw material diameter
     * @param workpiece The workpiece shape to encompass
     * @param axis The rotation axis for the raw material
     */
    void displayRawMaterialForWorkpiece(double diameter, const TopoDS_Shape& workpiece, const gp_Ax1& axis);

    /**
     * @brief Display raw material for a transformed workpiece with auto-sizing
     * @param diameter Raw material diameter in mm
     * @param workpiece The workpiece shape to encompass
     * @param axis The rotation axis for the cylinder
     * @param transform The transformation applied to the workpiece
     */
    void displayRawMaterialForWorkpieceWithTransform(double diameter, const TopoDS_Shape& workpiece, const gp_Ax1& axis, const gp_Trsf& transform);

    /**
     * @brief Find the next largest standard diameter for a given diameter
     */
    double getNextStandardDiameter(double diameter);

    /**
     * @brief Get list of all standard diameters
     */
    const QVector<double>& getStandardDiameters() const { return STANDARD_DIAMETERS; }

    /**
     * @brief Set custom diameter with workpiece-based length calculation
     */
    void setCustomDiameter(double diameter, const TopoDS_Shape& workpiece, const gp_Ax1& axis);

    /**
     * @brief Clear all raw material from the scene
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

    /**
     * @brief Get the current raw material diameter
     * @return Current diameter in mm, or 0.0 if no raw material is displayed
     */
    double getCurrentDiameter() const { return m_currentDiameter; }

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
    
    double m_currentDiameter;
    
    /**
     * @brief Create a cylinder shape for raw material
     */
    TopoDS_Shape createCylinder(double diameter, double length, const gp_Ax1& axis);
    
    /**
     * @brief Create a cylinder shape that properly encompasses a workpiece
     */
    TopoDS_Shape createCylinderForWorkpiece(double diameter, double length, const gp_Ax1& axis, const TopoDS_Shape& workpiece);
    
    /**
     * @brief Create a cylinder shape that properly encompasses a transformed workpiece
     */
    TopoDS_Shape createCylinderForWorkpieceWithTransform(double diameter, double length, const gp_Ax1& axis, const TopoDS_Shape& workpiece, const gp_Trsf& transform);
    
    /**
     * @brief Calculate optimal length for raw material based on workpiece bounds
     */
    double calculateOptimalLength(const TopoDS_Shape& workpiece, const gp_Ax1& axis);
    
    /**
     * @brief Calculate optimal length for raw material based on transformed workpiece bounds
     */
    double calculateOptimalLengthWithTransform(const TopoDS_Shape& workpiece, const gp_Ax1& axis, const gp_Trsf& transform);
    
    /**
     * @brief Set raw material visual properties
     */
    void setRawMaterialMaterial(Handle(AIS_Shape) rawMaterialAIS);
};

#endif // RAWMATERIALMANAGER_H 