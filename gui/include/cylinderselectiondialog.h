#ifndef CYLINDERSELECTIONDIALOG_H
#define CYLINDERSELECTIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVector>

// Forward declaration
struct CylinderInfo;

/**
 * @brief Dialog for manual cylinder axis selection
 * 
 * This dialog allows users to manually select which detected cylinder
 * should be used as the main turning axis for workpiece alignment.
 */
class CylinderSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CylinderSelectionDialog(const QVector<CylinderInfo>& cylinders, 
                                   int currentSelection = 0,
                                   QWidget *parent = nullptr);
    ~CylinderSelectionDialog();

    /**
     * @brief Get the index of the selected cylinder
     * @return Index of selected cylinder, or -1 if cancelled
     */
    int getSelectedCylinderIndex() const;

    /**
     * @brief Get information about the selected cylinder
     * @return CylinderInfo structure of selected cylinder
     */
    CylinderInfo getSelectedCylinderInfo() const;

private slots:
    /**
     * @brief Handle selection change in the cylinder list
     */
    void onSelectionChanged();

    /**
     * @brief Handle OK button click
     */
    void onOkClicked();

    /**
     * @brief Handle Cancel button click
     */
    void onCancelClicked();

private:
    // UI components
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QLabel* m_instructionLabel;
    QListWidget* m_cylinderList;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Data
    QVector<CylinderInfo> m_cylinders;
    int m_selectedIndex;
    
    /**
     * @brief Set up the user interface
     */
    void setupUI();
    
    /**
     * @brief Populate the cylinder list with detected cylinders
     */
    void populateCylinderList();
    
    /**
     * @brief Update the selection and button states
     */
    void updateSelection();
};

#endif // CYLINDERSELECTIONDIALOG_H 