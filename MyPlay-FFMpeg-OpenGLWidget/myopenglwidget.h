#ifndef MYOPENGLWIDGET_H
#define MYOPENGLWIDGET_H

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
    enImageTexture = 0,
    enRGBATexture,
    enRGBTexture,
    enYUVTexture,
};

enum GraphicsType
{
    enRectangle = 0,  //矩形
    enRectRing,       //矩形环
    enSphere,         //球体
    enSquare,         //正方形
    enCube,           //立方体
};

enum PlayState
{
    enClose = 0,
    enOpen,
    enPlay,
    enPause,
};


class MyOpenGLWidget : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT

public:
    MyOpenGLWidget(QWidget *pParent = NULL);
    ~MyOpenGLWidget();

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

    void setVideoFormat(int iPixelFormat, int iWidth, int iHeight);  //AVPixelFormat: AV_PIX_FMT_YUV420P=0; AV_PIX_FMT_RGB24=2;
    void updateVideoData(unsigned char *pYUVFrame);
    void updatePlayState(int iState);
    //void enablePlayButton(bool bEnable);

public slots:
    void OnUpdatePlayState(enum PlayState iState);  //响应信号：sig_updatePlayState
    void OnUpdateMyWindow();

signals:
    void sig_updatePlayState(int iState);  //向上层应用报告状态
    void sig_updateMyWindow();  //用于内部update();

protected:
    virtual void resizeGL(int w, int h);
    virtual void initializeGL();
    virtual void paintGL();

    void initShaders();
    void initVertexCube();
    void initVertexRectRing();
    void initTexture();

    void paintGL_TextureProgram();
    void drawCube();

    void updateMyWindow();

private:
    int m_iVertexCount;
    int m_iVertexRectRing;
    int m_iIndexRectRing;
    enum TextureType m_enTextureType;
    enum GraphicsType m_enGraphicsType;  //图形类型：矩形、立方体、梯型环...
    bool m_bQuartering;  //是否四分显示

    GLuint m_uTextureId[3];
    QOpenGLTexture *m_pTexture;
    QOpenGLVertexArrayObject *m_pVAOCube;
    QOpenGLVertexArrayObject *m_pVAORectRing;
    QOpenGLShaderProgram *m_pShaderProgram;
    QOpenGLShaderProgram *m_pShaderProgramYUV;

    QImage m_imageTexture;
    bool m_bUpdateTexture;
    bool m_bUpdateTextureYUV;

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
