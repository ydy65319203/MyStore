#include "myopenglwidget.h"
#include "mylog.h"

#include <QVBoxLayout>

//AVPixelFormat
const int AV_PIX_FMT_YUV420P = 0;
const int AV_PIX_FMT_RGB24 = 2;

MyOpenGLWidget::MyOpenGLWidget(QWidget *pParent)
    : QOpenGLWidget(pParent)
{
    Q_UNUSED(AV_PIX_FMT_YUV420P);
    Q_UNUSED(AV_PIX_FMT_RGB24);

    m_iVertexCount = 0;
    m_iVertexRectRing = 0;
    m_iIndexRectRing = 0;

    m_enTextureType = enImageTexture;
    m_enGraphicsType = enRectRing;  //矩形环
    //m_enGraphicsType = enCube;    //立方体

    m_bQuartering = false;

    memset(m_uTextureId, 0, sizeof(m_uTextureId));

    m_pTexture = NULL;
    m_pVAOCube = NULL;
    m_pVAORectRing = NULL;
    m_pShaderProgram = NULL;
    m_pShaderProgramYUV = NULL;

    m_bUpdateTexture = false;
    m_bUpdateTextureYUV = false;

    m_pYUVFrame = NULL;
    m_iYFrameSize = 0;
    m_iUFrameSize = 0;
    m_iHeight = 0;
    m_iWidth = 0;

    m_fAngleX = 0.0f;
    m_fAngleY = 0.0f;
    m_fAngleZ = 0.0f;

    connect(this, &MyOpenGLWidget::sig_updateMyWindow, this, &MyOpenGLWidget::OnUpdateMyWindow);
}

MyOpenGLWidget::~MyOpenGLWidget()
{

}

void MyOpenGLWidget::initControlPanel(CMyWidgetControlPanel *pWidgetControlPanel)
{
    LOG(Info, "MyOpenGLWidget::initControlPanel( pWidgetControlPanel = %p )... \n", pWidgetControlPanel);
    m_pWidgetControlPanel = pWidgetControlPanel;

    QVBoxLayout *pVBoxLayoutMain = new QVBoxLayout;
    pVBoxLayoutMain->addStretch();
    pVBoxLayoutMain->addWidget((QWidget*)pWidgetControlPanel);

    this->setLayout(pVBoxLayoutMain);
}

void MyOpenGLWidget::initControlPanel(CMyFrameControlPanel *pFrameControlPanel)
{
    LOG(Info, "MyOpenGLWidget::initControlPanel( pFrameControlPanel = %p )... \n", pFrameControlPanel);
    m_pFrameControlPanel = pFrameControlPanel;

    QVBoxLayout *pVBoxLayoutMain = new QVBoxLayout;
    pVBoxLayoutMain->addStretch();
    pVBoxLayoutMain->addWidget((QWidget*)pFrameControlPanel);

    this->setLayout(pVBoxLayoutMain);
}

void MyOpenGLWidget::setQuartering()
{
    m_bQuartering = true;
    update();
}

void MyOpenGLWidget::setGraphicsTypeCube()
{
    m_enGraphicsType = enCube;
    m_bQuartering = false;
    update();
}

void MyOpenGLWidget::setGraphicsTypePlane()
{
    qDebug("MyOpenGLWidget::setGraphicsTypePlane()---> setPlayState(enOpen); ");
    this->updatePlayState(enOpen);
}

void MyOpenGLWidget::setGraphicsTypeRectRing()
{
    m_enGraphicsType = enRectRing;
    m_bQuartering = false;
    update();
}

void MyOpenGLWidget::rotateX(float fAngle)
{
//    qDebug("MyOpenGLWidget::rotateX(fAngle=%f)...", fAngle);
    float fRotate = fAngle - m_fAngleX;
    m_Matrix4MVP.rotate(fRotate, 1.0f, 0, 0);
    m_fAngleX = fAngle;
    update();
}

void MyOpenGLWidget::rotateY(float fAngle)
{
    //qDebug("MyOpenGLWidget::rotateY(fAngle=%f)...", fAngle);
    float fRotate = fAngle - m_fAngleY;
    m_Matrix4MVP.rotate(fRotate, 0, 1.0f, 0);
    m_fAngleY = fAngle;
    update();
}

void MyOpenGLWidget::rotateZ(float fAngle)
{
    //qDebug("MyOpenGLWidget::rotateZ(fAngle=%f)...", fAngle);
    float fRotate = fAngle - m_fAngleZ;
    m_Matrix4MVP.rotate(fRotate, 0, 0, 1.0f);
    m_fAngleZ = fAngle;
    update();
}

void MyOpenGLWidget::setImageTexture(QImage &imageTexture)
{
    m_enTextureType = enImageTexture;  //设置纹理类型
    m_imageTexture = imageTexture;     //传入QImage对象
    m_bUpdateTexture = true;
}

void MyOpenGLWidget::setYUVTexture(unsigned char *pYUVFrame, int iWidth, int iHeight)
{
    m_enTextureType = enYUVTexture;    //设置纹理类型
    m_bUpdateTextureYUV = true;
    m_pYUVFrame = pYUVFrame;
    m_iHeight = iHeight;
    m_iWidth = iWidth;

    //m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
    //m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1

    update();
}

void MyOpenGLWidget::updateYUVTexture(unsigned char *pYUVFrame)
{
    m_bUpdateTextureYUV = true;
    m_pYUVFrame = pYUVFrame;
    update();
}

void MyOpenGLWidget::updatePlayState(int iState)
{
    LOG(Info, "MyOpenGLWidget::updatePlayState()---> emit sig_updatePlayState(iState=%d); \n", iState);
    emit sig_updatePlayState(iState);
}

//AVPixelFormat: AV_PIX_FMT_YUV420P=0; AV_PIX_FMT_RGB24=2;
void MyOpenGLWidget::setVideoFormat(int iPixelFormat, int iWidth, int iHeight)
{
    if(iPixelFormat == AV_PIX_FMT_YUV420P)
    {
        //this->resize(iWidth, iHeight);
        m_iHeight = iHeight;
        m_iWidth  = iWidth;
    }
}

void MyOpenGLWidget::updateVideoData(unsigned char *pYUVFrame)
{
    m_enTextureType = enYUVTexture;    //设置纹理类型
    m_bUpdateTextureYUV = true;
    m_pYUVFrame = pYUVFrame;
    
    //刷新窗口
    updateMyWindow();
}


//发出信号: sig_updateMyWindow
void MyOpenGLWidget::updateMyWindow()
{
    //LOG(Info, "MyOpenGLWidget::updateMyWindow()---> emit sig_updateMyWindow(); \n");
    emit sig_updateMyWindow();
}

//槽函数,响应信号: sig_updateMyWindow
void MyOpenGLWidget::OnUpdateMyWindow()
{
    //LOG(Info, "MyOpenGLWidget::OnUpdateMyWindow()---> update(); \n");
    update();
}

//槽函数
void MyOpenGLWidget::OnUpdatePlayState(enum PlayState iState)
{
    LOG(Info, "MyOpenGLWidget::OnUpdatePlayState( iState = %d )... \n", iState);
}


void MyOpenGLWidget::resizeGL(int w, int h)
{
    m_iScreenW = w;
    m_iScreenH = h;
}

void MyOpenGLWidget::initializeGL()
{
//    QGLFormat format;
//    format.setVersion(2,2);
//    format.setProfile(QGLFormat::CoreProfile);

    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    initShaders();
    initVertexCube();
    initVertexRectRing();
    //initTexture();

//    QOpenGLBuffer *pPBO = new QOpenGLBuffer(QOpenGLBuffer::PixelPackBuffer);
//    pPBO->create();
//    pPBO->bind();
//    pPBO->allocate(m_iWidth * m_iHeight);

    GLint iDefaultFrameBuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &iDefaultFrameBuffer);
    qDebug("MyOpenGLWidget::initializeGL()---> glGetIntegerv(GL_FRAMEBUFFER_BINDING, &iDefaultFrameBuffer) = %d;", iDefaultFrameBuffer);
}

void MyOpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.2f, 0.0f, 1.0f);
    paintGL_TextureProgram();

    if(m_bQuartering)  //四分割
    {
        //左上
        glViewport(0, m_iScreenH/2, m_iScreenW/2, m_iScreenH/2);
        if(m_enGraphicsType == enCube)  //立方体
        {
            m_pVAOCube->bind();
            glDrawArrays(GL_TRIANGLES, 0, m_iVertexCount);
        }
        else if(m_enGraphicsType == enRectRing)  //矩形环
        {
            m_pVAORectRing->bind();
            glDrawElements(GL_TRIANGLE_STRIP, m_iIndexRectRing, GL_UNSIGNED_SHORT, 0);
        }

        //右下
        glViewport(m_iScreenW/2, 0, m_iScreenW/2, m_iScreenH/2);
        if(m_enGraphicsType == enCube)  //立方体
        {
            m_pVAOCube->bind();
            glDrawArrays(GL_TRIANGLES, 0, m_iVertexCount);
        }
        else if(m_enGraphicsType == enRectRing)  //矩形环
        {
            m_pVAORectRing->bind();
            glDrawElements(GL_TRIANGLE_STRIP, m_iIndexRectRing, GL_UNSIGNED_SHORT, 0);
        }
    }
    else
    {
        glViewport(0, 0, m_iScreenW, m_iScreenH);
    }

    //绘制
    if(m_enGraphicsType == enCube)  //立方体
    {
        m_pVAOCube->bind();
        glDrawArrays(GL_TRIANGLES, 0, m_iVertexCount);
    }
    else if(m_enGraphicsType == enRectRing)  //矩形环
    {
        m_pVAORectRing->bind();
        glDrawElements(GL_TRIANGLE_STRIP, m_iIndexRectRing, GL_UNSIGNED_SHORT, 0);
    }

    //释放着色器纹理
    if(m_enTextureType == enImageTexture)
    {
        m_pShaderProgram->release();
        m_pTexture->release();
    }
    else if(m_enTextureType == enYUVTexture)
    {
        m_pShaderProgramYUV->release();
    }

    //释放VAO
    if(m_enGraphicsType == enCube)  //立方体
    {
        m_pVAOCube->release();
    }
    else if(m_enGraphicsType == enRectRing)  //矩形环
    {
        m_pVAORectRing->release();
    }
}

//初始化着色器
void MyOpenGLWidget::initShaders()
{
    m_pShaderProgram = new QOpenGLShaderProgram();
    m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/textureSampler2D.fsh");
    m_pShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertexTextureMatrix.vsh");
    bool bRet = m_pShaderProgram->link();
    if(bRet)
    {
        qDebug("MyOpenGLWidget::initShaders()---> m_pShaderProgram->link() succ.");
    }
    else
    {
        qDebug("MyOpenGLWidget::initShaders()---> m_pShaderProgram->link() fail.");
    }

    m_pShaderProgramYUV = new QOpenGLShaderProgram();
    m_pShaderProgramYUV->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertexTextureMatrix.vsh");
    m_pShaderProgramYUV->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/textureSampler2D_YUV.fsh");
    bool bRetYUV = m_pShaderProgramYUV->link();
    if(bRetYUV)
    {
        qDebug("MyOpenGLWidget::initShaders()---> m_pShaderProgramYUV->link() succ.");
    }
    else
    {
        qDebug("MyOpenGLWidget::initShaders()---> m_pShaderProgramYUV->link() fail.");
    }
}

//立方体
void MyOpenGLWidget::initVertexCube()
{
    GLfloat GLfVertexCube[] = {
        //前面
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  //左下
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  //右下
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  //右上
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  //左上
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  //左下

        //后面
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  //左下
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  //右下
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  //右上
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  //左上
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  //左下

        //左面
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  //后下
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  //前下
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  //前上
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  //前上
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  //后上
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  //后下

        //右面
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  //前上
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  //后上
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  //后下
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  //前下
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  //前上

        //底面
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        //顶面
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    m_pVAOCube = new QOpenGLVertexArrayObject();
    m_pVAOCube->create();
    m_pVAOCube->bind();

    QOpenGLBuffer *pVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    pVBO->create();
    pVBO->bind();
    pVBO->allocate(GLfVertexCube, sizeof(GLfVertexCube));

    m_pShaderProgram->bind();
    m_pShaderProgram->setAttributeBuffer("vec2TexurePos", GL_FLOAT, 3*sizeof(GLfloat), 2, (3+2)*sizeof(GLfloat));
    m_pShaderProgram->setAttributeBuffer("vec3VertexPos", GL_FLOAT, 0, 3, (3+2)*sizeof(GLfloat));
    m_pShaderProgram->enableAttributeArray(m_pShaderProgram->attributeLocation("vec3VertexPos"));
    m_pShaderProgram->enableAttributeArray(m_pShaderProgram->attributeLocation("vec2TexurePos"));

    m_iVertexCount = sizeof(GLfVertexCube)/sizeof(GLfVertexCube[0])/5;
    qDebug("MyOpenGLWidget::initVertexCube()---> sizeof(GLfVertexCube) = %d; m_iVertexCount = %d;", sizeof(GLfVertexCube), m_iVertexCount);

//    QOpenGLFunctions *pOpenGLFunctions = this->context()->functions();
//    pOpenGLFunctions->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3+2)*sizeof(GLfloat), 0);
//    pOpenGLFunctions->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (3+2)*sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
//    pOpenGLFunctions->glEnableVertexAttribArray(0);
//    pOpenGLFunctions->glEnableVertexAttribArray(1);
}


//矩形环
void MyOpenGLWidget::initVertexRectRing()
{
    const int iScaleA = 2;
    const int iScaleB = 3;
    const int iScaleAZ = 2;
    const int iScaleBZ = 2;

    GLfloat GLfVertexRectRing[] = {
//        //顶点          //纹理
//        1,  1.732, 0,  0.0f, 0.0f,
//        2,     0,  0,  1.0f, 0.0f,
//        1, -1.732, 0,  0.0f, 0.0f,
//       -1, -1.732, 0,  1.0f, 0.0f,
//       -2,      0, 0,  0.0f, 0.0f,
//       -1,  1.732, 0,  1.0f, 0.0f,

        1.0f/iScaleA,  1.732/iScaleA,  1.0/iScaleAZ,  0.0f, 0.0f,
        2.0f/iScaleA,              0,  1.0/iScaleAZ,  1.0f, 0.0f,
        1.0f/iScaleA, -1.732/iScaleA,  1.0/iScaleAZ,  0.0f, 0.0f,
       -1.0/iScaleA,  -1.732/iScaleA,  1.0/iScaleAZ,  1.0f, 0.0f,
       -2.0/iScaleA,               0,  1.0/iScaleAZ,  0.0f, 0.0f,
       -1.0/iScaleA,   1.732/iScaleA,  1.0/iScaleAZ,  1.0f, 0.0f,

        1.0/iScaleB,  1.732/iScaleB,  -1.0/iScaleBZ,   0.0f, 1.0f,
        2.0/iScaleB,      0/iScaleB,  -1.0/iScaleBZ,   1.0f, 1.0f,
        1.0/iScaleB, -1.732/iScaleB,  -1.0/iScaleBZ,   0.0f, 1.0f,
       -1.0/iScaleB, -1.732/iScaleB,  -1.0/iScaleBZ,   1.0f, 1.0f,
       -2.0/iScaleB,      0/iScaleB,  -1.0/iScaleBZ,   0.0f, 1.0f,
       -1.0/iScaleB,  1.732/iScaleB,  -1.0/iScaleBZ,   1.0f, 1.0f,
                  0,              0,  -1.0/iScaleBZ,   0.0f, 0.0f,
    };

    GLushort GluIndexRectRing[] = {0, 6, 1,  7,  2,  8,  3,  9,  4,  10,  5, 11, 0, 6,   //桶壁
                                  // 12, 7, 8, 12, 9, 10  //桶底
                                  };

    m_pVAORectRing = new QOpenGLVertexArrayObject();
    m_pVAORectRing->create();
    m_pVAORectRing->bind();

    QOpenGLBuffer *pVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    pVBO->create();
    pVBO->bind();
    pVBO->allocate(GLfVertexRectRing, sizeof(GLfVertexRectRing));

    QOpenGLBuffer *pEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    pEBO->create();
    pEBO->bind();
    pEBO->setUsagePattern(QOpenGLBuffer::StaticDraw);
    pEBO->allocate(GluIndexRectRing, sizeof(GluIndexRectRing));

    m_pShaderProgramYUV->bind();
    m_pShaderProgramYUV->setAttributeBuffer("vec2TexurePos", GL_FLOAT, 3*sizeof(GLfloat), 2, (3+2)*sizeof(GLfloat));
    m_pShaderProgramYUV->setAttributeBuffer("vec3VertexPos", GL_FLOAT, 0, 3, (3+2)*sizeof(GLfloat));
    m_pShaderProgramYUV->enableAttributeArray(m_pShaderProgram->attributeLocation("vec3VertexPos"));
    m_pShaderProgramYUV->enableAttributeArray(m_pShaderProgram->attributeLocation("vec2TexurePos"));

    m_iVertexRectRing = sizeof(GLfVertexRectRing)/sizeof(GLfVertexRectRing[0])/5;
    m_iIndexRectRing = sizeof(GluIndexRectRing)/sizeof(GluIndexRectRing[0]);
    qDebug("MyOpenGLWidget::initVertexRectRing()---> sizeof(GLfVertexRectRing)=%d; m_iVertexRectRing=%d; m_iIndexRectRing=%d;", sizeof(GLfVertexRectRing), m_iVertexRectRing, m_iIndexRectRing);
}

//初始化纹理
void MyOpenGLWidget::initTexture()
{
    if(m_imageTexture.load(":/smileface.png"))
    {
        m_pTexture = new QOpenGLTexture(m_imageTexture.mirrored());
        m_enTextureType = enImageTexture;
        m_bUpdateTexture  = false;
    }

//    m_pTextureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
//    m_pTextureY->create();
//    GLuint TextureY = m_pTextureY->textureId();

//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, TextureY);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_nVideoW, m_nVideoH, 0, GL_RED, GL_UNSIGNED_BYTE, m_pBufYuv420p);
}

//选择着色器
void MyOpenGLWidget::paintGL_TextureProgram()
{
    if(m_enTextureType == enImageTexture)  //图片纹理
    {
        //创建Image纹理
        if(m_pTexture == NULL) //(m_bCreatedTexture == false && m_bUpdateTexture == true)
        {
            if(m_bUpdateTexture)
            {
                qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> Create image Texture.");
                m_pTexture = new QOpenGLTexture(m_imageTexture.mirrored());
                m_bUpdateTexture  = false;
            }
            else if(m_imageTexture.load(":/smileface.png"))
            {
                qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> Create default image Texture.");
                m_pTexture = new QOpenGLTexture(m_imageTexture.mirrored());
                m_bUpdateTexture  = false;
            }
        }

        if(m_pTexture)
        {
            //更新Image纹理
            if(m_bUpdateTexture)
            {
                qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> Modify image Texture.");

                //销毁
                m_pTexture->destroy();
                delete m_pTexture;

                //重建
                m_pTexture = new QOpenGLTexture(m_imageTexture.mirrored());
                m_bUpdateTexture  = false;
            }

            //绑定纹理对象
            m_pTexture->bind(0);

            //绑定着色器
            if(m_pShaderProgram)
            {
                m_pShaderProgram->bind();
                m_pShaderProgram->setUniformValue("texSampler2D", 0);
                m_pShaderProgram->setUniformValue("mat4MVP", m_Matrix4MVP);
            }
        }
    }
    else if(m_enTextureType == enYUVTexture)  //YUV纹理
    {
        //创建YUV纹理
        if(m_uTextureId[1] == 0 && m_uTextureId[2] == 0 && m_pYUVFrame)
        {
            m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
            m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1

            qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> Create YUV Texture.");
            m_bUpdateTextureYUV = false;
            glGenTextures(3, m_uTextureId);
            qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> m_uTextureId[0]=%u, m_uTextureId[1]=%u, m_uTextureId[2]=%u;", m_uTextureId[0], m_uTextureId[1], m_uTextureId[2]);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);  // 为当前绑定的纹理对象设置环绕、过滤方式
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // 环绕方式x方向 重复  //GL_CLAMP_TO_EDGE //GL_CLAMP_TO_BORDER
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // y 方向重复
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // 纹理缩小时的过滤方式 线性
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // 纹理放大时的过滤方式 线性 or GL_NEAREST 相邻
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth, m_iHeight, 0, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);

            //qDebug("MyOpenGLWidget::paintCube()---> glActiveTexture(GL_TEXTURE1);");
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);

            //qDebug("MyOpenGLWidget::paintCube()---> glActiveTexture(GL_TEXTURE2);");
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
        }
        else if(m_bUpdateTextureYUV)  //更新YUV纹理
        {
            //qDebug("MyOpenGLWidget::paintCube()---> Update YUV Texture.");

            m_bUpdateTextureYUV = false;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth, m_iHeight, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth/2, m_iHeight/2, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth/2, m_iHeight/2, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
        }
        else  //绑定纹理
        {
            //qDebug("MyOpenGLWidget::paintCube()---> repaint YUV Texture.");

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
        }

        //绑定着色器
        if(m_pShaderProgramYUV)
        {
            m_pShaderProgramYUV->bind();
            m_pShaderProgramYUV->setUniformValue("texSampler2D_Y", 0);
            m_pShaderProgramYUV->setUniformValue("texSampler2D_U", 1);
            m_pShaderProgramYUV->setUniformValue("texSampler2D_V", 2);
            m_pShaderProgramYUV->setUniformValue("mat4MVP", m_Matrix4MVP);
        }
    }
}

//绘制立方体
void MyOpenGLWidget::drawCube()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.2f, 0.0f, 1.0f);

    //    QMatrix4x4 matrix;
    //    matrix.perspective(45.0f, (GLfloat)w/(GLfloat)h, 0.1f, 100.0f);
    //    matrix.translate(0, 0, translate);
    //    matrix.rotate(-60, 0, 1, 0);  //绕Y轴逆时针旋转
    //    matrix.rotate(xRot / 16.0f, 1.0f, 0.0f, 0.0f);
    //    matrix.rotate(yRot / 16.0f, 0.0f, 1.0f, 0.0f);
    //    matrix.rotate(zRot / 16.0f, 0.0f, 0.0f, 1.0f);
    //    program->setUniformValue("matrix", matrix);

    if(m_pTexture)
    {
        if(m_bUpdateTexture)
        {
            m_pTexture->setData(m_imageTexture);
            m_bUpdateTexture = false;
        }

        m_pTexture->bind(0);
        m_pShaderProgram->bind();
        m_pShaderProgram->setUniformValue("texSampler2D", 0);
        m_pShaderProgram->setUniformValue("mat4MVP", m_Matrix4MVP);
    }

    m_pVAOCube->bind();
    glDrawArrays(GL_TRIANGLES, 0, m_iVertexCount);
    //glDrawArrays(GL_POINTS, 0, 36);
    //glDrawArrays(GL_LINES, 0, 36);

    m_pShaderProgram->release();
    m_pVAOCube->release();
}

