#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mylog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);  // Qt::FramelessWindowHint | Qt::Tool
    //setAttribute(Qt::WA_TranslucentBackground);
    //this->setWindowOpacity(0.5);

    LOG(Info, "MainWindow::MainWindow()...\r\n");
    ui->setupUi(this);

    m_pFrameControlPanel = new CMyFrameControlPanel;
    this->setCentralWidget(m_pFrameControlPanel);
    this->setObjectName("MainWindow");
}

MainWindow::~MainWindow()
{
    if (m_pFrameControlPanel)
    {
        delete m_pFrameControlPanel;
    }

    delete ui;
}

