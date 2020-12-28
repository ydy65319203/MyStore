#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mylog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	LOG(Info, "MainWindow::MainWindow()...\r\n");
    ui->setupUi(this);

    m_pFrameControlPanel = new CMyFrameControlPanel;
    this->setCentralWidget(m_pFrameControlPanel);
}

MainWindow::~MainWindow()
{
    if (m_pFrameControlPanel)
    {
        delete m_pFrameControlPanel;
    }

    delete ui;
}

