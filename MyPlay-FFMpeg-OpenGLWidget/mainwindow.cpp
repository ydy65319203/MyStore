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

    //标题栏显示文件路径
    connect(m_pFrameControlPanel, &CMyFrameControlPanel::sig_setMainWindowTitle, this, &MainWindow::setMainWindowTitle);
    m_qstrMessage = "YDY Player   Ver1.0    65319203@qq.com";
    this->setWindowTitle(m_qstrMessage);
    m_iTimerId = this->startTimer(500);
}

MainWindow::~MainWindow()
{
//    if (m_pFrameControlPanel)
//    {
//        delete m_pFrameControlPanel;
//    }

    delete ui;
}

void MainWindow::setMainWindowTitle(QString &qstrTitle)
{
    if(m_iTimerId > 0)
    {
        this->killTimer(m_iTimerId);
        m_iTimerId = 0;
    }

    this->setWindowTitle(qstrTitle);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_iTimerId)
    {
        this->setWindowTitle(m_qstrMessage);
    }
}

