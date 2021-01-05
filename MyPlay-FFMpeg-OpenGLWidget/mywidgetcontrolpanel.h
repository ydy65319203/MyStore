#ifndef CMYWIDGETCONTROLPANEL_H
#define CMYWIDGETCONTROLPANEL_H

#include "myffmpeg.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimerEvent>

#include <QFileDialog>
#include <QFileInfo>

#include <QLineEdit>
#include <QSlider>
#include <QWidget>

class CMyWidgetControlPanel : public QWidget
{
    Q_OBJECT
public:
    CMyWidgetControlPanel();
    MyOpenGLWidget* getMyOpenGLWidget();
    //void setFFmpeg(CMyFFmpeg *pMyFFmpeg);

public slots:
    void OnVideoPlayState(int iState);
    void OnAudioPlayState(int iState);
    void OnButton_OpenFile();
    void OnButton_Play();

protected:
      void timerEvent(QTimerEvent *event) override;

private:
    void openYUVFile(QString &qstrFileName);
    void parseYUVFileName(QString &qstrFileName, int &iWidth, int &iHeight);
    void playYUVFrame();

private:    
    MyOpenGLWidget *m_pMyOpenGLWidget;     //OpenGL绘图
    CMyAudioOutput *m_pMyAudioOutput;      //播放音频
    CMyFFmpeg      *m_pMyFFmpeg;

    //-----------------------------

    QPushButton  *m_pPushButton_Play;      //播放按钮
    QPushButton  *m_pPushButton_Pause;     //暂停按钮
    QPushButton  *m_pPushButton_OpenFile;  //打开文件
    QLineEdit    *m_pLineEdit_FilePath;    //显示文件路径

    QPushButton *m_pPushButton_SplitWindow;   //分割窗口
    QPushButton *m_pPushButton_Cube;   //立方体
    QPushButton *m_pPushButton_Ring;   //梯形环
    QPushButton *m_pPushButton_Plane;  //平面

    QSlider *m_pSliderPlay;     //播放进度条
    QSlider *m_pSliderX;
    QSlider *m_pSliderY;
    QSlider *m_pSliderZ;

    QImage m_imageTexture;

    //-----------------------------

    QString m_qstrImageFile;
    QString m_qstrFileSuffix;  //文件名后缀 .yuv

    qint64 m_iYUVFrameSize;
    qint64 m_iYUVFileSize;
    qint64 m_iPlayPos;
    QFile *m_pFileYUV;
    unsigned char *m_pYUVBuffer;
    int m_iPixelHeigth;
    int m_iPixelWidth;

    int m_iYUVTimerId;

    int m_iVideoPlayState;
    int m_iAudioPlayState;
};

#endif // CMYWIDGETCONTROLPANEL_H
