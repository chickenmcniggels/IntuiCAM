#include "cylinderselectiondialog.h"
#include "workpiecemanager.h" // For CylinderInfo

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QListWidgetItem>
#include <QFont>

CylinderSelectionDialog::CylinderSelectionDialog(const QVector<CylinderInfo>& cylinders, 
                                               int currentSelection,
                                               QWidget *parent)
    : QDialog(parent)
    , m_cylinders(cylinders)
    , m_selectedIndex(currentSelection)
{
    setupUI();
    populateCylinderList();
    updateSelection();
}

CylinderSelectionDialog::~CylinderSelectionDialog()
{
    // Qt handles cleanup automatically
}

int CylinderSelectionDialog::getSelectedCylinderIndex() const
{
    return m_selectedIndex;
}

CylinderInfo CylinderSelectionDialog::getSelectedCylinderInfo() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_cylinders.size()) {
        return m_cylinders[m_selectedIndex];
    }
    
    // Return invalid info if no valid selection
    return CylinderInfo(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 0.0, 0.0, "Invalid");
}

void CylinderSelectionDialog::setupUI()
{
    setWindowTitle("Select Turning Axis");
    setModal(true);
    setMinimumSize(400, 300);
    resize(450, 350);
    
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Title label
    m_titleLabel = new QLabel("Multiple Cylindrical Features Detected");
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel);
    
    // Instruction label
    m_instructionLabel = new QLabel("Please select which cylinder should be used as the main turning axis:");
    m_instructionLabel->setWordWrap(true);
    m_mainLayout->addWidget(m_instructionLabel);
    
    // Cylinder list
    m_cylinderList = new QListWidget();
    m_cylinderList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cylinderList->setAlternatingRowColors(true);
    m_mainLayout->addWidget(m_cylinderList);
    
    // Button layout
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    
    m_okButton = new QPushButton("OK");
    m_okButton->setDefault(true);
    m_okButton->setMinimumWidth(80);
    m_buttonLayout->addWidget(m_okButton);
    
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setMinimumWidth(80);
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_cylinderList, &QListWidget::currentRowChanged,
            this, &CylinderSelectionDialog::onSelectionChanged);
    connect(m_cylinderList, &QListWidget::itemDoubleClicked,
            this, &CylinderSelectionDialog::onOkClicked);
    connect(m_okButton, &QPushButton::clicked,
            this, &CylinderSelectionDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked,
            this, &CylinderSelectionDialog::onCancelClicked);
}

void CylinderSelectionDialog::populateCylinderList()
{
    m_cylinderList->clear();
    
    for (int i = 0; i < m_cylinders.size(); ++i) {
        const CylinderInfo& cylinder = m_cylinders[i];
        
        // Create list item with detailed information
        QString itemText = QString("%1\nDiameter: %2 mm, Length: %3 mm")
                          .arg(cylinder.description)
                          .arg(QString::number(cylinder.diameter, 'f', 1))
                          .arg(QString::number(cylinder.estimatedLength, 'f', 1));
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, i); // Store index
        
        // Add visual indicators
        if (i == 0) {
            // Highlight the largest cylinder (default selection)
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            item->setBackground(QColor(230, 245, 255)); // Light blue background
        }
        
        m_cylinderList->addItem(item);
    }
}

void CylinderSelectionDialog::updateSelection()
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_cylinderList->count()) {
        m_cylinderList->setCurrentRow(m_selectedIndex);
    }
    
    m_okButton->setEnabled(m_selectedIndex >= 0);
}

void CylinderSelectionDialog::onSelectionChanged()
{
    int currentRow = m_cylinderList->currentRow();
    if (currentRow >= 0 && currentRow < m_cylinders.size()) {
        m_selectedIndex = currentRow;
    } else {
        m_selectedIndex = -1;
    }
    
    m_okButton->setEnabled(m_selectedIndex >= 0);
}

void CylinderSelectionDialog::onOkClicked()
{
    if (m_selectedIndex >= 0) {
        accept();
    }
}

void CylinderSelectionDialog::onCancelClicked()
{
    m_selectedIndex = -1;
    reject();
} 