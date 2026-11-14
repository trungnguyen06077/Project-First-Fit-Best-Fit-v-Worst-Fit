#include "MainWindow.hpp"
#include "MemoryView.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    this->setWindowTitle("Mo Phong Cap Phat Bo Nho - Giao Dien Chinh");
    this->resize(900, 650);

    txtHolesPath = new QLineEdit(this);
    txtHolesPath->setReadOnly(true);
    txtHolesPath->setPlaceholderText("Chua chon file holes.csv...");
    btnSelectHoles = new QPushButton("Chon File Holes", this);

    txtRequestsPath = new QLineEdit(this);
    txtRequestsPath->setReadOnly(true);
    txtRequestsPath->setPlaceholderText("Chua chon file requests.csv...");
    btnSelectRequests = new QPushButton("Chon File Requests", this);

    txtOutputDirPath = new QLineEdit(this);
    txtOutputDirPath->setReadOnly(true);
    txtOutputDirPath->setText(".");
    btnSelectOutputDir = new QPushButton("Thu Muc Xuat CSV", this);

    comboAlgo = new QComboBox(this);
    for (bo_nho::Algorithm a : bo_nho::kAllAlgorithms) {
        comboAlgo->addItem(bo_nho::displayName(a), static_cast<int>(a));
    }

    btnRunSim = new QPushButton("Chay Mo Phong", this);
    btnRunSim->setStyleSheet("font-weight: bold; background-color: #2b78e4; color: white; padding: 6px;");

    lblStatus = new QLabel("Trang thai: San sang. Hay nap day du file du lieu.", this);
    lblMetrics = new QLabel("Hieu nang: Thoi gian: 0 us | Thanh cong: 0 | That bai: 0 | Con trong: 0 KB", this);

    memoryView = new MemoryView(this);

    tableResult = new QTableWidget(this);
    tableResult->setColumnCount(8);
    QStringList headers = {"Step", "Process ID", "Size Req (KB)", "Allocated", "Block ID", "Alloc Start", "Size Before", "Size After"};
    tableResult->setHorizontalHeaderLabels(headers);
    tableResult->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QVBoxLayout *mainLayout = new QVBoxLayout();

    QHBoxLayout *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("File Holes:", this), 1);
    row1->addWidget(txtHolesPath, 6);
    row1->addWidget(btnSelectHoles, 2);
    mainLayout->addLayout(row1);

    QHBoxLayout *row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("File Requests:", this), 1);
    row2->addWidget(txtRequestsPath, 6);
    row2->addWidget(btnSelectRequests, 2);
    mainLayout->addLayout(row2);

    QHBoxLayout *row3 = new QHBoxLayout();
    row3->addWidget(new QLabel("Thu muc ra:", this), 1);
    row3->addWidget(txtOutputDirPath, 6);
    row3->addWidget(btnSelectOutputDir, 2);
    mainLayout->addLayout(row3);

    QHBoxLayout *row4 = new QHBoxLayout();
    row4->addWidget(new QLabel("Chon Chien Luoc Cap Phat:", this));
    row4->addWidget(comboAlgo, 2);
    row4->addSpacing(20);
    row4->addWidget(btnRunSim, 2);
    mainLayout->addLayout(row4);

    mainLayout->addWidget(lblStatus);
    mainLayout->addWidget(memoryView);
    mainLayout->addWidget(tableResult, 1);
    mainLayout->addWidget(lblMetrics);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    this->setCentralWidget(centralWidget);

    connect(btnSelectHoles, &QPushButton::clicked, this, &MainWindow::handleSelectHolesFile);
    connect(btnSelectRequests, &QPushButton::clicked, this, &MainWindow::handleSelectRequestsFile);
    connect(btnSelectOutputDir, &QPushButton::clicked, this, &MainWindow::handleSelectOutputDir);
    connect(btnRunSim, &QPushButton::clicked, this, &MainWindow::handleRunSimulation);
}

void MainWindow::handleSelectHolesFile() {
    QString file = QFileDialog::getOpenFileName(this, "Chon file o trong bo nho", "", "CSV Files (*.csv)");
    if (!file.isEmpty()) txtHolesPath->setText(file);
}

void MainWindow::handleSelectRequestsFile() {
    QString file = QFileDialog::getOpenFileName(this, "Chon file yeu cau cap phat", "", "CSV Files (*.csv)");
    if (!file.isEmpty()) txtRequestsPath->setText(file);
}

void MainWindow::handleSelectOutputDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Chon thu muc luu ket qua xuat ban", "");
    if (!dir.isEmpty()) txtOutputDirPath->setText(dir);
}

void MainWindow::handleRunSimulation() {
    QString holesFile = txtHolesPath->text();
    QString reqFile = txtRequestsPath->text();
    QString outDir = txtOutputDirPath->text();

    if (holesFile.isEmpty() || reqFile.isEmpty()) {
        QMessageBox::warning(this, "Thieu du lieu", "Vui long chon day du ca hai file du lieu dau vao!");
        return;
    }

    std::vector<bo_nho::Hole> holes;
    std::vector<bo_nho::Request> reqs;
    std::string err;

    lblStatus->setText("Trang thai: Dang doc va kiem tra du lieu CSV...");
    if (!bo_nho::readBlocks(holesFile.toStdString(), holes, err) || 
        !bo_nho::readRequests(reqFile.toStdString(), reqs, err)) {
        QMessageBox::critical(this, "Loi file CSV", QString::fromStdString(err));
        lblStatus->setText("Trang thai: Loi nap file.");
        return;
    }

    auto e1 = bo_nho::kiemTraDanhSachHole(holes);
    auto e2 = bo_nho::kiemTraDanhSachYeuCau(reqs);
    if (!e1.empty() || !e2.empty()) {
        std::string fullErr = bo_nho::taoThongBaoLoi(holesFile.toStdString(), e1) + 
                             bo_nho::taoThongBaoLoi(reqFile.toStdString(), e2);
        QMessageBox::critical(this, "Du lieu khong hop le", QString::fromStdString(fullErr));
        lblStatus->setText("Trang thai: Du lieu vi pham quy tac kiem dinh.");
        return;
    }

    bo_nho::Algorithm selectedAlgo = static_cast<bo_nho::Algorithm>(comboAlgo->currentData().toInt());

    lblStatus->setText(QString("Trang thai: Dang mo phong thuat toan %1...").arg(bo_nho::displayName(selectedAlgo)));
    bo_nho::SimulationResult result = bo_nho::simulate(holes, reqs, selectedAlgo);

    updateTableDisplay(result);
    memoryView->datDuLieu(holes, result.allocations);

    QString metricsText = QString("Thoi gian: %1 us | Thanh cong: %2 | That bai: %3 | Con trong: %4 KB (Lon nhat: %5 KB)")
                          .arg(result.metrics.timeUs)
                          .arg(result.metrics.allocated)
                          .arg(result.metrics.failed)
                          .arg(result.metrics.totalFree)
                          .arg(result.metrics.largestFree);
    lblMetrics->setText(metricsText);
    lblStatus->setText("Trang thai: Mo phong hoan tat!");

    std::string outDirStd = outDir.toStdString();
    std::string algoName = bo_nho::toString(selectedAlgo);
    bo_nho::writeTrace(outDirStd + "/trace_" + algoName + ".csv", result.trace, err);
    std::vector<bo_nho::Metrics> singleSummary = {result.metrics};
    bo_nho::writeSummary(outDirStd + "/summary_" + algoName + ".csv", singleSummary, err);
}

void MainWindow::updateTableDisplay(const bo_nho::SimulationResult& sr) {
    tableResult->setRowCount(0);

    for (int i = 0; i < (int)sr.trace.size(); ++i) {
        const bo_nho::TraceRecord& tr = sr.trace[i];
        tableResult->insertRow(i);

        tableResult->setItem(i, 0, new QTableWidgetItem(QString::number(tr.step)));
        tableResult->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(tr.processId)));
        tableResult->setItem(i, 2, new QTableWidgetItem(QString::number(tr.requestSize)));
        tableResult->setItem(i, 3, new QTableWidgetItem(tr.allocated ? "Success" : "Failed"));
        
        if (tr.allocated) {
            tableResult->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(tr.blockId)));
            tableResult->setItem(i, 5, new QTableWidgetItem(QString::number(tr.allocStart)));
            tableResult->setItem(i, 6, new QTableWidgetItem(QString::number(tr.sizeBefore)));
            tableResult->setItem(i, 7, new QTableWidgetItem(QString::number(tr.sizeAfter)));
            
            for(int col=0; col<8; ++col) {
                tableResult->item(i, col)->setBackground(QColor(230, 245, 230));
            }
        } else {
            tableResult->setItem(i, 4, new QTableWidgetItem("-"));
            tableResult->setItem(i, 5, new QTableWidgetItem("-"));
            tableResult->setItem(i, 6, new QTableWidgetItem("-"));
            tableResult->setItem(i, 7, new QTableWidgetItem("-"));
            
            for(int col=0; col<8; ++col) {
                tableResult->item(i, col)->setBackground(QColor(255, 230, 230));
            }
        }
    }
  }
