#ifndef PARTLOADINGPANEL_H
#define PARTLOADINGPANEL_H

#include <QWidget>
#include <QGroupBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>

// OpenCASCADE includes
#include <gp_Ax1.hxx>
#include <TopoDS_Shape.hxx>

// Forward declarations
struct CylinderInfo;

/**
 * @brief Modern part loading control panel
 * 
 * This panel provides user-friendly controls for:
 * - Automatic axis detection and alignment
 * - Distance to chuck adjustment
 * - Raw material diameter control
 * - Part orientation flipping
 * - Manual axis selection from detected cylinders
 */
class PartLoadingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PartLoadingPanel(QWidget *parent = nullptr);
    ~PartLoadingPanel();

    /**
     * @brief Update panel with detected cylinder information
     */
    void updateCylinderInfo(const QVector<CylinderInfo>& cylinders);

    /**
     * @brief Set the current workpiece for analysis
     */
    void setWorkpiece(const TopoDS_Shape& workpiece);

    /**
     * @brief Get current distance to chuck setting
     */
    double getDistanceToChuck() const;

    /**
     * @brief Get current raw material diameter
     */
    double getRawMaterialDiameter() const;

    /**
     * @brief Get current orientation flip state
     */
    bool isOrientationFlipped() const;

    /**
     * @brief Get selected cylinder index (-1 for automatic)
     */
    int getSelectedCylinderIndex() const;

    /**
     * @brief Reset panel to default state
     */
    void reset();

signals:
    /**
     * @brief Emitted when distance to chuck changes
     */
    void distanceToChuckChanged(double distance);

    /**
     * @brief Emitted when raw material diameter changes
     */
    void rawMaterialDiameterChanged(double diameter);

    /**
     * @brief Emitted when orientation is flipped
     */
    void orientationFlipped(bool flipped);

    /**
     * @brief Emitted when a different cylinder is selected
     */
    void cylinderSelectionChanged(int index);

    /**
     * @brief Emitted when manual axis selection is requested
     */
    void manualAxisSelectionRequested();

private slots:
    void onDistanceSliderChanged(int value);
    void onDistanceSpinBoxChanged(double value);
    void onRawMaterialDiameterChanged(double value);
    void onOrientationFlipToggled(bool checked);
    void onCylinderSelectionChanged(int index);
    void onManualAxisSelectionClicked();

private:
    void setupUI();
    void setupPositioningGroup();
    void setupMaterialGroup();
    void setupAxisGroup();
    void updateDistanceControls(double distance);
    void updateCylinderComboBox();
    void updateAxisInfo(const CylinderInfo& info);

    // UI Components
    QVBoxLayout* m_mainLayout;
    
    // Part positioning group
    QGroupBox* m_positioningGroup;
    QLabel* m_distanceLabel;
    QSlider* m_distanceSlider;
    QDoubleSpinBox* m_distanceSpinBox;
    QCheckBox* m_flipOrientationCheckBox;
    
    // Raw material group
    QGroupBox* m_materialGroup;
    QLabel* m_diameterLabel;
    QDoubleSpinBox* m_rawMaterialDiameterSpinBox;
    QLabel* m_materialLengthLabel;
    
    // Axis selection group
    QGroupBox* m_axisGroup;
    QComboBox* m_cylinderComboBox;
    QPushButton* m_manualAxisButton;
    QLabel* m_axisInfoLabel;
    
    // Data
    QVector<CylinderInfo> m_detectedCylinders;
    TopoDS_Shape m_currentWorkpiece;
    bool m_updating;
};

#endif // PARTLOADINGPANEL_H 