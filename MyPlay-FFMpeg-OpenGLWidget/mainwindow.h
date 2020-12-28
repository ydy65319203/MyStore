#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "myframecontrolpanel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
	CMyFrameControlPanel *m_pFrameControlPanel;  //播放器控制面板
};
#endif // MAINWINDOW_H
