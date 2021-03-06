﻿#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "myframecontrolpanel.h"
#include "mywidgetcontrolpanel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void setMainWindowTitle(QString & qstrTitle);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    Ui::MainWindow *ui;

    CMyFrameControlPanel *m_pFrameControlPanel;  //播放器控制面板
    CMyWidgetControlPanel *m_pWidgetControlPanel;

    QString m_qstrMessage;
    int m_iTimerId;
};
#endif // MAINWINDOW_H
