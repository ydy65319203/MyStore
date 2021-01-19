#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mylog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //setWindowFlags(Qt::FramelessWindowHint);  // Qt::FramelessWindowHint | Qt::Tool
    //setAttribute(Qt::WA_TranslucentBackground);
    //this->setWindowOpacity(0.5);

    LOG(Info, "MainWindow::MainWindow()... \n");
    this->setObjectName("MainWindow");
    ui->setupUi(this);

    //---------------------------------------------------

    //控制面板做中央控件，包含显示控件
    m_pFrameControlPanel = new CMyFrameControlPanel;
    this->setCentralWidget(m_pFrameControlPanel->getMyOpenGLWidget());
    this->setStatusBar(NULL);  //没有状态栏

    //显示控件做中央控件，包含控制面板
//    m_pWidgetControlPanel = new CMyWidgetControlPanel;
//    this->setCentralWidget(m_pWidgetControlPanel->getMyOpenGLWidget());
//    this->setStatusBar(NULL);  //没有状态栏

    this->setWindowTitle("MyPlay Ver1.0");  //标题栏显示文件路径
    connect(m_pFrameControlPanel, &CMyFrameControlPanel::sig_setPlayMessage, this, &MainWindow::setWindowTitle);
}

MainWindow::~MainWindow()
{
//    if (m_pFrameControlPanel)
//    {
//        delete m_pFrameControlPanel;
//    }

    delete ui;
}

