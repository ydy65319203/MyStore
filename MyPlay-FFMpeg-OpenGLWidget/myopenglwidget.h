#ifndef MYOPENGLWIDGET_H
#define MYOPENGLWIDGET_H

//#include "mywidgetcontrolpanel.h"
class CMyWidgetControlPanel;
class CMyFrameControlPanel;

#include <QMouseEvent>
#include <QFrame>

#include <QOpenGLWidget>

#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

#include <QMatrix4x4>
//#include <QVector3D>
//#include <QVector>
//#include <QTime>

#include <QImage>

enum TextureType
{
    enImageTextureCreate=0,
    enImageTextureUpdate,
    enImageTextureShow,

    enYUVTextureCreate,
    enYUVTextureUpdate,
    enYUVTextureShow,

    enRGBATexture,
    enRGBTexture,
};

enum GraphicsType
{
    enRectangle = 0,  //矩形
    enRectRing,       //梯形环
    enSphere,         //球体
    enCube,           //立方体
    enPlane,
};

enum PlayState
{
    //播放按钮状态
    enClose = 0,
    enPause,
    enPlay,

    //FFMpeg通知上层应用
    enOpenSucc,
    enOpenFail,

    enPlayEnd,
    //enPlayIng,
    //enPlayFail,
};


class MyOpenGLWidget : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT

public:
    MyOpenGLWidget(QWidget *pParent = NULL);
    ~MyOpenGLWidget();

    void initControlPanel(CMyWidgetControlPanel *pWidgetControlPanel);
    void initControlPanel(CMyFrameControlPanel *pFrameControlPanel);

    //void splitWindow();  //拆分窗口
    void setQuartering();
    void setGraphicsTypeCube();
    void setGraphicsTypePlane();
    void setGraphicsTypeRectRing();

    void rotateX(float fAngle = 10.0f);
    void rotateY(float fAngle = 10.0f);
    void rotateZ(float fAngle = 10.0f);

    void setImageTexture(QImage &imageTexture);
    void setYUVTexture(unsigned char *pYUVFrame, int iWidth, int iHeight);
    void updateYUVTexture(unsigned char *pYUVFrame);

    void setVideoFormat(int iPixelFormat, int iWidth, int iHeight, int64_t iVideoStreamDuration, unsigned char *pData);  //AVPixelFormat: AV_PIX_FMT_YUV420P=0; AV_PIX_FMT_RGB24=2;
    void updateVideoData(unsigned char *pYUVFrame, int64_t iPts, int64_t iDuration);
    void updatePlayState(int iState, std::string & sstrMessage);  //更新播放状态

    void setReportFlag(bool bReport);  //设置上报标志
    void setVideoStreamDuration(int iNum, int iDen, int64_t iVideoStreamDuration);

public slots:
    void OnSetVideoFormat(int iPixelFormat, int iWidth, int iHeight, unsigned char *pData);
    void OnUpdateMyWindow();

signals:
    void sig_updatePlayStep(int iStep, int iReportTotal);  //向上层报告播放进度
    void sig_updatePlayState(int iState, const char *pszMessage);  //向上层应用报告状态
    void sig_setVideoFormat(int iPixelFormat, int iWidth, int iHeight, unsigned char *pData);  //用于内部同步纹理格式
    void sig_updateMyWindow();  //用于内部update();

protected:
    virtual void resizeGL(int w, int h) override;
    virtual void initializeGL() override;
    virtual void paintGL() override;

    void initShaders();
    void initVertexCube();
    void initVertexPlane();
    void initVertexRectRing();
    void initTexture();

    void paintGL_TextureProgram();
    void drawGraphics();  //绘图
    void drawCube();      //废弃

    void updateMyWindow();

protected:
    //重写Widget的一些方法  //实现窗口可拖动
    //void mousePressEvent(QMouseEvent *event) override;
    //void mouseMoveEvent(QMouseEvent *event) override;
    //void mouseReleaseEvent(QMouseEvent *event) override;

    void enterEvent(QEvent *event) override;  //进入QWidget瞬间事件
    void leaveEvent(QEvent *event) override;  //离开QWidget瞬间事件

    //关闭时不退出，而是到系统托盘
    //void closeEvent(QCloseEvent *event) override;

    //拖拽文件进入
    //void dragEnterEvent(QDragEnterEvent* event) override;
    //void dropEvent(QDropEvent* event) override;

private:
    CMyWidgetControlPanel *m_pWidgetControlPanel;  //播放器的控制面板
    CMyFrameControlPanel *m_pFrameControlPanel;  //播放器的控制面板
    QRect m_rectControlPanel;  //控制面板位置
    bool m_bControlPanel;      //控制面板this->isVisible();

    bool m_bReportStep;
    int m_iReportTotal;
    int m_iReportInterval;  //根据时间基计算上报间隔
    int64_t m_iReportDuration;
    int64_t m_iPts;

    int m_iVertexCount;
    int m_iVertexCount_Plane;
    int m_iVertexRectRing;
    int m_iIndexRectRing;
    enum TextureType m_enTextureType;
    enum GraphicsType m_enGraphicsType;  //图形类型：矩形、立方体、梯型环...
    bool m_bQuartering;  //是否四分显示

    GLuint m_uTextureId[3];
    QOpenGLTexture *m_pTexture;
    QOpenGLVertexArrayObject *m_pVAOCube;
    QOpenGLVertexArrayObject *m_pVAOPlane;
    QOpenGLVertexArrayObject *m_pVAORectRing;
    QOpenGLShaderProgram *m_pShaderProgram;
    QOpenGLShaderProgram *m_pShaderProgramYUV;

    QImage m_imageTexture;
    //bool m_bUpdateTexture;
    //bool m_bUpdateTextureYUV;

    unsigned char *m_pYUVFrame;  //一帧YUV数据
    int m_iYFrameSize;  //Y分量的像素数量
    int m_iUFrameSize;
    int m_iHeight;
    int m_iWidth;

    int m_iScreenW;
    int m_iScreenH;

    QMatrix4x4 m_Matrix4MVP;
    float m_fAngleX;
    float m_fAngleY;
    float m_fAngleZ;
};

#endif // MYOPENGLWIDGET_H
