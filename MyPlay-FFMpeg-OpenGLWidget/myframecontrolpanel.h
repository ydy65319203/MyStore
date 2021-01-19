#ifndef CMYFRAMECONTROLPANEL_H
#define CMYFRAMECONTROLPANEL_H

#include "myopenglwidget.h"
#include "myffmpeg.h"

#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimerEvent>

#include <QMouseEvent>

#include <QFileDialog>
#include <QFileInfo>

#include <QLineEdit>
#include <QSlider>
#include <QFrame>

class CMyFrameControlPanel : public QFrame
{
    Q_OBJECT
public:
    CMyFrameControlPanel();
    ~CMyFrameControlPanel();
    MyOpenGLWidget* getMyOpenGLWidget();

signals:
    void sig_setPlayMessage(QString & qstrMessage);

public slots:
    void OnVideoPlayStep(int iStep, int iVideoReportTotal);
    void OnAudioPlayStep(int iStep, int iAudioReportTotal);
    void OnVideoPlayState(int iState);
    void OnAudioPlayState(int iState);
    void OnButton_OpenFile();
    void OnButton_Play();

    void OnButton_Voice();

    void startAudio();  //临时测试。界面不知道声音，由FFmpeg解码到音频流后发起播放。

protected:
    void timerEvent(QTimerEvent *event) override;

    //重写Widget的一些方法  //实现窗口可拖动
    //void mousePressEvent(QMouseEvent *event) override;
    //void mouseMoveEvent(QMouseEvent *event) override;
    //void mouseReleaseEvent(QMouseEvent *event) override;

    //void enterEvent(QEvent *event) override;  //进入QWidget瞬间事件
    //void leaveEvent(QEvent *event) override;  //离开QWidget瞬间事件

    //关闭时不退出，而是到系统托盘
    //void closeEvent(QCloseEvent *event) override;

    //拖拽文件进入
    //void dragEnterEvent(QDragEnterEvent* event) override;
    //void dropEvent(QDropEvent* event) override;

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
    QPushButton *m_pPushButton_Voice;  //音量

    QSlider *m_pSliderVolume;    //音量条
    QSlider *m_pSliderPlay;      //播放进度条
    QSlider *m_pSliderX;
    QSlider *m_pSliderY;
    QSlider *m_pSliderZ;

    QImage m_imageTexture;

    //-----------------------------

    QString m_qstrFilePath;    //文件全路径名
    QString m_qstrFileName;    //文件全名(带扩展名)
    QString m_qstrFileSuffix;  //文件名后缀 .yuv
    QString m_qstrMessage;     //外发提示消息

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

    int m_iVideoReportTotal;  //播放进度条总步长
    int m_iAudioReportTotal;
};

#endif // CMYFRAMECONTROLPANEL_H
