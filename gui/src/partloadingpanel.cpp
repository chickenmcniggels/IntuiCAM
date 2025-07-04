#include "partloadingpanel.h"
#include "workpiecemanager.h" // For CylinderInfo

#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QFont>
#include <QDebug>

PartLoadingPanel::PartLoadingPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_positioningGroup(nullptr)
    , m_distanceLabel(nullptr)
    , m_distanceSlider(nullptr)
    , m_distanceSpinBox(nullptr)
    , m_flipOrientationCheckBox(nullptr)
    , m_materialGroup(nullptr)
    , m_diameterLabel(nullptr)
    , m_rawMaterialDiameterSpinBox(nullptr)
    , m_materialLengthLabel(nullptr)
    , m_axisGroup(nullptr)
    , m_manualAxisButton(nullptr)
    , m_axisInfoLabel(nullptr)
    , m_updating(false)
{
    setupUI();
    reset();
}

PartLoadingPanel::~PartLoadingPanel()
{
    // Qt handles cleanup automatically
}

void PartLoadingPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);

    // Add informational label about auto-apply
    QLabel* autoApplyLabel = new QLabel("Changes are applied automatically");
    autoApplyLabel->setStyleSheet(
        "QLabel {"
        "  color: #666;"
        "  font-size: 11px;"
        "  font-style: italic;"
        "  padding: 4px 8px;"
        "  background-color: #f0f0f0;"
        "  border-radius: 4px;"
        "  border: 1px solid #ddd;"
        "}"
    );
    autoApplyLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(autoApplyLabel);

    // Create the three main groups
    setupPositioningGroup();
    setupMaterialGroup();
    setupAxisGroup();

    // Add all components to main layout
    m_mainLayout->addWidget(m_positioningGroup);
    m_mainLayout->addWidget(m_materialGroup);
    m_mainLayout->addWidget(m_axisGroup);
    m_mainLayout->addStretch();

    // Connect signals
    connect(m_distanceSlider, &QSlider::valueChanged,
            this, &PartLoadingPanel::onDistanceSliderChanged);
    connect(m_distanceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PartLoadingPanel::onDistanceSpinBoxChanged);
    connect(m_rawMaterialDiameterSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PartLoadingPanel::onRawMaterialDiameterChanged);
    connect(m_flipOrientationCheckBox, &QCheckBox::toggled,
            this, &PartLoadingPanel::onOrientationFlipToggled);
    connect(m_manualAxisButton, &QPushButton::clicked,
            this, &PartLoadingPanel::onManualAxisSelectionClicked);
}

void PartLoadingPanel::setupPositioningGroup()
{
    m_positioningGroup = new QGroupBox("Part Positioning");
    m_positioningGroup->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 2px solid #cccccc;"
        "  border-radius: 8px;"
        "  margin-top: 1ex;"
        "  padding-top: 12px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 8px 0 8px;"
        "}"
    );

    QVBoxLayout* groupLayout = new QVBoxLayout(m_positioningGroup);
    groupLayout->setSpacing(12);

    // Distance to chuck control
    QLabel* distanceTitle = new QLabel("Distance to Chuck:");
    distanceTitle->setStyleSheet("font-weight: bold; color: #333;");
    
    m_distanceLabel = new QLabel("25.0 mm");
    m_distanceLabel->setStyleSheet("color: #666; font-size: 12px;");
    m_distanceLabel->setAlignment(Qt::AlignRight);

    QHBoxLayout* distanceTitleLayout = new QHBoxLayout();
    distanceTitleLayout->addWidget(distanceTitle);
    distanceTitleLayout->addStretch();
    distanceTitleLayout->addWidget(m_distanceLabel);

    // Slider and spinbox in horizontal layout
    m_distanceSlider = new QSlider(Qt::Horizontal);
    m_distanceSlider->setRange(0, 500); // 0-500mm range
    m_distanceSlider->setSingleStep(2); // faster scroll increments
    m_distanceSlider->setPageStep(20);  // larger jumps with page keys
    m_distanceSlider->setValue(25); // Default 25mm
    m_distanceSlider->setTickPosition(QSlider::TicksBelow);
    m_distanceSlider->setTickInterval(20);

    m_distanceSpinBox = new QDoubleSpinBox();
    m_distanceSpinBox->setRange(0.0, 500.0);
    m_distanceSpinBox->setValue(25.0);
    m_distanceSpinBox->setSuffix(" mm");
    m_distanceSpinBox->setDecimals(1);
    m_distanceSpinBox->setMaximumWidth(80);

    QHBoxLayout* distanceControlLayout = new QHBoxLayout();
    distanceControlLayout->addWidget(m_distanceSlider, 1);
    distanceControlLayout->addWidget(m_distanceSpinBox);

    // Flip orientation checkbox
    m_flipOrientationCheckBox = new QCheckBox("Flip Part Orientation");
    m_flipOrientationCheckBox->setStyleSheet("font-weight: normal; color: #333;");

    groupLayout->addLayout(distanceTitleLayout);
    groupLayout->addLayout(distanceControlLayout);
    groupLayout->addWidget(m_flipOrientationCheckBox);
}

void PartLoadingPanel::setupMaterialGroup()
{
    m_materialGroup = new QGroupBox("Raw Material");
    m_materialGroup->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 2px solid #cccccc;"
        "  border-radius: 8px;"
        "  margin-top: 1ex;"
        "  padding-top: 12px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 8px 0 8px;"
        "}"
    );

    QFormLayout* groupLayout = new QFormLayout(m_materialGroup);
    groupLayout->setVerticalSpacing(12);
    groupLayout->setHorizontalSpacing(16);

    // Raw material diameter
    m_diameterLabel = new QLabel("Diameter:");
    m_diameterLabel->setStyleSheet("font-weight: bold; color: #333;");

    m_rawMaterialDiameterSpinBox = new QDoubleSpinBox();
    m_rawMaterialDiameterSpinBox->setRange(5.0, 500.0);
    m_rawMaterialDiameterSpinBox->setValue(50.0);
    m_rawMaterialDiameterSpinBox->setSuffix(" mm");
    m_rawMaterialDiameterSpinBox->setDecimals(1);
    m_rawMaterialDiameterSpinBox->setMaximumWidth(120);

    // Material length (calculated automatically)
    m_materialLengthLabel = new QLabel("Length: Auto-calculated");
    m_materialLengthLabel->setStyleSheet("color: #666; font-size: 12px;");

    groupLayout->addRow(m_diameterLabel, m_rawMaterialDiameterSpinBox);
    groupLayout->addRow("", m_materialLengthLabel);
}

void PartLoadingPanel::setupAxisGroup()
{
    m_axisGroup = new QGroupBox("Rotational Axis");
    m_axisGroup->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 2px solid #cccccc;"
        "  border-radius: 8px;"
        "  margin-top: 1ex;"
        "  padding-top: 12px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 8px 0 8px;"
        "}"
    );

    QVBoxLayout* groupLayout = new QVBoxLayout(m_axisGroup);
    groupLayout->setSpacing(12);

    // Manual axis selection button (only way to select axis)
    m_manualAxisButton = new QPushButton("Select Rotational Axis from 3D View");
    m_manualAxisButton->setMinimumHeight(32);
    m_manualAxisButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #1565C0;"
        "}"
    );

    // Axis information
    m_axisInfoLabel = new QLabel("Click the button above, then select a cylindrical surface or circular edge in the 3D view");
    m_axisInfoLabel->setStyleSheet("color: #666; font-size: 11px;");
    m_axisInfoLabel->setWordWrap(true);

    groupLayout->addWidget(m_manualAxisButton);
    groupLayout->addWidget(m_axisInfoLabel);
}

void PartLoadingPanel::updateCylinderInfo(const QVector<CylinderInfo>& cylinders)
{
    m_detectedCylinders = cylinders;
    
    // Update the UI to show information about detected cylinders
    if (!cylinders.isEmpty()) {
        QString detectedInfo = QString("Detected %1 cylindrical feature(s). ")
                              .arg(cylinders.size());
        detectedInfo += "Click 'Select Rotational Axis from 3D View' to choose which one to use for turning.";
        m_axisInfoLabel->setText(detectedInfo);
    } else {
        m_axisInfoLabel->setText("No cylindrical features detected. Click 'Select Rotational Axis from 3D View' to manually select geometry.");
    }
}

void PartLoadingPanel::setWorkpiece(const TopoDS_Shape& workpiece)
{
    m_currentWorkpiece = workpiece;
}

double PartLoadingPanel::getDistanceToChuck() const
{
    return m_distanceSpinBox->value();
}

double PartLoadingPanel::getRawMaterialDiameter() const
{
    return m_rawMaterialDiameterSpinBox->value();
}

bool PartLoadingPanel::isOrientationFlipped() const
{
    return m_flipOrientationCheckBox->isChecked();
}



void PartLoadingPanel::reset()
{
    m_updating = true;

    // Reset positioning controls
    m_distanceSlider->setValue(25);
    m_distanceSpinBox->setValue(25.0);
    m_flipOrientationCheckBox->setChecked(false);

    // Reset material controls
    m_rawMaterialDiameterSpinBox->setValue(50.0);
    m_materialLengthLabel->setText("Length: Auto-calculated");

    // Reset axis controls
    m_axisInfoLabel->setText("Click the button above, then select a cylindrical surface or circular edge in the 3D view");

    // Clear data
    m_detectedCylinders.clear();
    m_currentWorkpiece = TopoDS_Shape();

    m_updating = false;
}

void PartLoadingPanel::onDistanceSliderChanged(int value)
{
    if (m_updating) return;
    
    m_updating = true;
    double distance = static_cast<double>(value);
    m_distanceSpinBox->setValue(distance);
    m_distanceLabel->setText(QString("%1 mm").arg(distance, 0, 'f', 1));
    m_updating = false;

    emit distanceToChuckChanged(distance);
}

void PartLoadingPanel::onDistanceSpinBoxChanged(double value)
{
    if (m_updating) return;

    m_updating = true;
    m_distanceSlider->setValue(static_cast<int>(value));
    m_distanceLabel->setText(QString("%1 mm").arg(value, 0, 'f', 1));
    m_updating = false;

    emit distanceToChuckChanged(value);
}

void PartLoadingPanel::onRawMaterialDiameterChanged(double value)
{
    if (m_updating) return;
    emit rawMaterialDiameterChanged(value);
}

void PartLoadingPanel::onOrientationFlipToggled(bool checked)
{
    if (m_updating) return;
    emit orientationFlipped(checked);
}

void PartLoadingPanel::onManualAxisSelectionClicked()
{
    emit manualAxisSelectionRequested();
}

void PartLoadingPanel::updateDistanceControls(double distance)
{
    m_updating = true;
    m_distanceSlider->setValue(static_cast<int>(distance));
    m_distanceSpinBox->setValue(distance);
    m_distanceLabel->setText(QString("%1 mm").arg(distance, 0, 'f', 1));
    m_updating = false;
}

void PartLoadingPanel::updateCylinderComboBox()
{
    // This method is no longer used since we removed the dropdown
    // but kept for backward compatibility - does nothing now
}

void PartLoadingPanel::updateAxisInfo(const CylinderInfo& info)
{
    gp_Pnt loc = info.axis.Location();
    gp_Dir dir = info.axis.Direction();
    
    QString infoText = QString("✓ Rotational Axis Selected:\n"
                              "Diameter: %1mm, Length: %2mm\n"
                              "Location: (%3, %4, %5)\n"
                              "Direction: (%6, %7, %8)\n"
                              "Workpiece has been aligned with Z-axis for turning.")
                      .arg(info.diameter, 0, 'f', 1)
                      .arg(info.estimatedLength, 0, 'f', 1)
                      .arg(loc.X(), 0, 'f', 1)
                      .arg(loc.Y(), 0, 'f', 1)
                      .arg(loc.Z(), 0, 'f', 1)
                      .arg(dir.X(), 0, 'f', 3)
                      .arg(dir.Y(), 0, 'f', 3)
                      .arg(dir.Z(), 0, 'f', 3);
    
    m_axisInfoLabel->setText(infoText);

} 