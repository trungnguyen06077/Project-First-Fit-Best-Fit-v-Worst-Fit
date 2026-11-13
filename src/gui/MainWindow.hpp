#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include "core/allocator.hpp"
#include "core/csv_io.hpp"
#include "core/models.hpp"
#include "core/validator.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleSelectHolesFile();
    void handleSelectRequestsFile();
    void handleSelectOutputDir();
    void handleRunSimulation();

private:
    void setupUI();
    void updateTableDisplay(const bo_nho::SimulationResult& sr);

    QLineEdit *txtHolesPath;
    QLineEdit *txtRequestsPath;
    QLineEdit *txtOutputDirPath;
    
    QPushButton *btnSelectHoles;
    QPushButton *btnSelectRequests;
    QPushButton *btnSelectOutputDir;
    QPushButton *btnRunSim;
    
    QComboBox *comboAlgo;
    QTableWidget *tableResult;
    
    QLabel *lblStatus;
    QLabel *lblMetrics;
};
