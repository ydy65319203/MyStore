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

    m_pWidgetControlPanel = NULL;
    m_pFrameControlPanel  = NULL;
    m_bControlPanel = true;

    m_bReportStep = false;
    m_iReportTotal = 0;
    m_iReportInterval = 0;
    m_iReportDuration = 0;

    m_iVertexCount = 0;
    m_iVertexCount_Plane = 0;
    m_iVertexRectRing = 0;
    m_iIndexRectRing  = 0;

    m_enTextureType = enImageTextureCreate;  //默认笑脸符
    m_enGraphicsType = enPlane;
    //m_enGraphicsType = enRectRing;  //矩形环
    //m_enGraphicsType = enCube;    //立方体
    m_bQuartering = false;

    memset(m_uTextureId, 0, sizeof(m_uTextureId));

    m_pTexture = NULL;
    m_pVAOCube = NULL;
    m_pVAOPlane = NULL;
    m_pVAORectRing = NULL;
    m_pShaderProgram = NULL;
    m_pShaderProgramYUV = NULL;

    //m_bUpdateTexture = false;
    //m_bUpdateTextureYUV = false;

    m_pYUVFrame = NULL;
    m_iYFrameSize = 0;
    m_iUFrameSize = 0;
    m_iHeight = 0;
    m_iWidth = 0;

    m_fAngleX = 0.0f;
    m_fAngleY = 0.0f;
    m_fAngleZ = 0.0f;

    connect(this, &MyOpenGLWidget::sig_setVideoFormat, this, &MyOpenGLWidget::OnSetVideoFormat);
    connect(this, &MyOpenGLWidget::sig_updateMyWindow, this, &MyOpenGLWidget::OnUpdateMyWindow);

    //追踪鼠标事件
    setMouseTracking(true);
    //setAttribute(Qt::WA_TransparentForMouseEvents,true);
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
    m_enGraphicsType = enPlane;
    m_bQuartering = false;
    update();
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
    m_enTextureType = enImageTextureCreate;  //设置纹理类型
    m_imageTexture = imageTexture;           //传入QImage对象
    //m_bUpdateTexture = true;
}

void MyOpenGLWidget::setYUVTexture(unsigned char *pYUVFrame, int iWidth, int iHeight)
{
    m_enTextureType = enYUVTextureCreate;    //设置纹理类型
    //m_bUpdateTextureYUV = true;
    m_pYUVFrame = pYUVFrame;
    m_iHeight = iHeight;
    m_iWidth = iWidth;

    //m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
    //m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1

    update();
}

void MyOpenGLWidget::updateYUVTexture(unsigned char *pYUVFrame)
{
    m_enTextureType = enYUVTextureUpdate;
    m_pYUVFrame = pYUVFrame;
    update();
}

void MyOpenGLWidget::setReportFlag(bool bReport)
{
    m_bReportStep = bReport;
}

void MyOpenGLWidget::setVideoStreamDuration(int iNum, int iDen, int64_t iVideoStreamDuration)
{
    //m_iVideoStreamDuration = iVideoStreamDuration;
    //m_iReportInterval = (iDen/iNum);  //上报间隔1秒

    //-----------------------------------------------------

    m_iReportInterval = (iDen/iNum);  //缺省上报间隔1秒
    m_iReportTotal = iVideoStreamDuration / m_iReportInterval;  //总时长(秒)
    int iHour = m_iReportTotal / 3600;         //小时
    int iMinute = m_iReportTotal % 3600 / 60;  //分钟
    int iSecond = m_iReportTotal % 60;         //秒

    //LOG(Info, "MyOpenGLWidget::setVideoStreamDuration()---> iVideoStreamDuration[0x%p] = [%02d:%02d:%02d]; \n", iVideoStreamDuration, iHour, iMinute, iSecond);

    if(m_iReportTotal <= (3600/2))
    {
        m_iReportInterval = (iDen/iNum)/4;  //上报间隔250毫秒
        m_iReportTotal = m_iReportTotal * 4;
    }
    else if((3600/2) < m_iReportTotal && m_iReportTotal < (3600*24*10))
    {
        //m_iReportInterval = m_iReportInterval;  //上报间隔1秒
        //m_iReportTotal = m_iReportTotal;
    }
    else
    {
        m_iReportInterval = m_iReportInterval * 16;
        m_iReportTotal = 0x7FFFFFFF;
    }

    LOG(Info, "MyOpenGLWidget::setVideoStreamDuration()---> iVideoStreamDuration[0x%X] = [%02d:%02d:%02d]; m_iReportInterval[pts] = %d, m_iReportTotal[step] = %d; \n",
              iVideoStreamDuration, iHour, iMinute, iSecond, m_iReportInterval, m_iReportTotal);
}

//AVPixelFormat: AV_PIX_FMT_YUV420P=0; AV_PIX_FMT_RGB24=2;
void MyOpenGLWidget::setVideoFormat(int iPixelFormat, int iWidth, int iHeight, int64_t iVideoStreamDuration, unsigned char *pData)
{
    //计算播放进度条间隔
    if(iVideoStreamDuration >= 1024)
    {
        //m_iReportTotal = 1024;
        m_iReportInterval = (iVideoStreamDuration / 1024);
        m_iReportTotal = (iVideoStreamDuration / m_iReportInterval) + 1;
    }
    else //if(iVideoStreamDuration < 1024)
    {
        m_iReportTotal = iVideoStreamDuration;
        m_iReportInterval = 1;
    }

    LOG(Info, "MyOpenGLWidget::setVideoFormat()---> iVideoStreamDuration=0x%X, m_iReportTotal=%d, m_iReportInterval=%d; \n", iVideoStreamDuration, m_iReportTotal, m_iReportInterval);

    LOG(Info, "MyOpenGLWidget::setVideoFormat()---> emit sig_setVideoFormat(iPixelFormat, iWidth, iHeight, pData); \n");
    emit sig_setVideoFormat(iPixelFormat, iWidth, iHeight, pData);
}

void MyOpenGLWidget::updateVideoData(unsigned char *pYUVFrame, int64_t iPts, int64_t iDuration)
{
    m_enTextureType = enYUVTextureUpdate;    //设置纹理类型
    //m_bUpdateTextureYUV = true;
    m_pYUVFrame = pYUVFrame;
    
    //刷新窗口
    updateMyWindow();

    //上报播放进度
//    if(m_bReportStep)
//    {
//        if((m_iPts+m_iReportInterval) < iPts)
//        {
//            m_iPts = iPts;  //保存Pts
//            int iStep = iPts / m_iReportInterval;
//            LOG(Debug, "MyOpenGLWidget::updateVideoData()---> iPts[%d] / m_iReportInterval[%d] = iStep[%d]; emit signal_updatePlayStep(iStep=%d, m_iReportTotal=%d); \n",
//                       iPts, m_iReportInterval, iStep, iStep, m_iReportTotal);
//            emit sig_updatePlayStep(iStep, m_iReportTotal);
//        }
//    }

    //上报播放进度
    if(m_bReportStep)
    {
        m_iReportDuration += iDuration;
        if(m_iReportDuration >= m_iReportInterval)
        {

            int iStep = iPts / m_iReportInterval;
            //LOG(Debug, "MyOpenGLWidget::updateVideoData()---> iPts[%d] / m_iReportInterval[%d] = iStep[%d]; emit signal_updatePlayStep(iStep=%d, m_iReportTotal=%d); \n", iPts, m_iReportInterval, iStep, iStep, m_iReportTotal);
            emit sig_updatePlayStep(iStep, m_iReportTotal);
            m_iReportDuration = 0;
        }
    }
}

void MyOpenGLWidget::updatePlayState(int iState, std::string & sstrMessage)
{
    LOG(Debug, "MyOpenGLWidget::updatePlayState(iState=%d, sstrMessage=%s)... \n", iState, sstrMessage.c_str());

    if(m_bReportStep)
    {
        LOG(Debug, "MyOpenGLWidget::updatePlayState()---> emit sig_updatePlayState(iState=%d, sstrMessage); \n", iState);
        emit sig_updatePlayState(iState, sstrMessage.c_str());
    }
}

//发出信号: sig_updateMyWindow
void MyOpenGLWidget::updateMyWindow()
{
    //LOG(Info, "MyOpenGLWidget::updateMyWindow()---> emit sig_updateMyWindow(); \n");
    emit sig_updateMyWindow();
}

//void MyOpenGLWidget::mouseMoveEvent(QMouseEvent *event)
//{
//    LOG(Debug, "MyOpenGLWidget::mouseMoveEvent(x=%d, y=%d)... \n", event->x(), event->y());

//    if(m_bControlPanel && event->y() <= 3)
//    {
//        //Hide控制面板
//        if(m_pFrameControlPanel)
//        {
//            LOG(Debug, "MyOpenGLWidget::mouseMoveEvent(x=%d, y=%d)---> m_pFrameControlPanel->hide(); \n", event->x(), event->y());
//            ((QFrame*)m_pFrameControlPanel)->hide();
//            m_bControlPanel = false;
//        }
//    }
//    else if(!m_bControlPanel && event->y() > m_rectControlPanel.y())
//    {
//        //Show控制面板
//        if(m_pFrameControlPanel)
//        {
//            LOG(Debug, "MyOpenGLWidget::mouseMoveEvent(x=%d, y=%d)---> m_pFrameControlPanel->show(); \n", event->x(), event->y());
//            ((QFrame*)m_pFrameControlPanel)->show();
//            m_bControlPanel = true;
//        }
//    }
//}

void MyOpenGLWidget::enterEvent(QEvent *event)
{
    //LOG(Debug, "MyOpenGLWidget::enterEvent( event->type() = %d )... \n", event->type());

    if(m_pFrameControlPanel)
    {
        //显示控制面板
        //LOG(Debug, "MyOpenGLWidget::leaveEvent()---> m_pFrameControlPanel->show(); \n");
        ((QFrame*)m_pFrameControlPanel)->show();
        m_bControlPanel = true;
    }

    Q_UNUSED(event);
}

void MyOpenGLWidget::leaveEvent(QEvent *event)
{
    //LOG(Debug, "MyOpenGLWidget::leaveEvent( event->type() = %d )... \n", event->type());

    if(m_bControlPanel && m_pFrameControlPanel)
    {
        //Hide控制面板
        //LOG(Debug, "MyOpenGLWidget::leaveEvent()---> m_pFrameControlPanel->hide(); \n");
        ((QFrame*)m_pFrameControlPanel)->hide();
        m_bControlPanel = false;
    }

    Q_UNUSED(event);
}

//槽函数,响应信号: sig_updateMyWindow
void MyOpenGLWidget::OnUpdateMyWindow()
{
    //LOG(Info, "MyOpenGLWidget::OnUpdateMyWindow()---> update(); \n");
    update();
}

void MyOpenGLWidget::OnSetVideoFormat(int iPixelFormat, int iWidth, int iHeight, unsigned char *pData)
{
    LOG(Info, "MyOpenGLWidget::OnSetVideoFormat(AV_PIX_FMT_YUV420P=0, width=%d, height=%d, pAVFrameYUV->data[0]=%p)... \n", iWidth, iHeight, pData);

    if(iPixelFormat == AV_PIX_FMT_YUV420P)
    {
        m_enTextureType = enYUVTextureCreate;    //设置纹理类型
        m_pYUVFrame = pData;
        m_iHeight = iHeight;
        m_iWidth = iWidth;

        //m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
        //m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1
    }

    //刷新窗口
    updateMyWindow();
}

//槽函数
//void MyOpenGLWidget::OnUpdatePlayState(enum PlayState iState)
//{
//    LOG(Info, "MyOpenGLWidget::OnUpdatePlayState( iState = %d )... \n", iState);
//}


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
    initVertexPlane();
    initVertexRectRing();
    //initTexture();

    if(m_imageTexture.load(":/picture/background-boxboy.jpg"))
    {
        LOG(Info, "MyOpenGLWidget::initializeGL()---> m_imageTexture.load(:/picture/background-boxboy.jpg) Succ. \n");
    }
    else
    {
        LOG(Warn, "MyOpenGLWidget::initializeGL()---> m_imageTexture.load(:/picture/background-boxboy.jpg) Fail. \n");
    }

//    QOpenGLBuffer *pPBO = new QOpenGLBuffer(QOpenGLBuffer::PixelPackBuffer);
//    pPBO->create();
//    pPBO->bind();
//    pPBO->allocate(m_iWidth * m_iHeight);

    GLint iDefaultFrameBuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &iDefaultFrameBuffer);
    LOG(Info, "MyOpenGLWidget::initializeGL()---> glGetIntegerv(GL_FRAMEBUFFER_BINDING, &iDefaultFrameBuffer) = %d; \n", iDefaultFrameBuffer);
}

void MyOpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.2f, 0.0f, 1.0f);
    paintGL_TextureProgram();

    //1.GL_TRIANGLES：每三个顶之间绘制三角形，之间不连接；
    //2.GL_TRIANGLE_FAN：以V0V1V2,V0V2V3,V0V3V4，……的形式绘制三角形；
    //3.GL_TRIANGLE_STRIP：以V0V1V2,V1V2V3,V2V3V4……的形式绘制三角形；

    if(m_bQuartering)  //四分割
    {
        //左上
        glViewport(0, m_iScreenH/2, m_iScreenW/2, m_iScreenH/2);
        this->drawGraphics();

        //右上
        glViewport(m_iScreenW/2, m_iScreenH/2, m_iScreenW/2, m_iScreenH/2);
        this->drawGraphics();

        //右下
        glViewport(m_iScreenW/2, 0, m_iScreenW/2, m_iScreenH/2);
        this->drawGraphics();

        //左下
        glViewport(0, 0, m_iScreenW/2, m_iScreenH/2);
        this->drawGraphics();
    }
    else
    {
        //设置输出窗口
        glViewport(0, 0, m_iScreenW, m_iScreenH);
        this->drawGraphics();
    }

    //释放着色器纹理
    if(m_enTextureType == enImageTextureShow)
    {
        m_pShaderProgram->release();
        m_pTexture->release();
    }
    else if(m_enTextureType == enYUVTextureShow)
    {
        m_pShaderProgramYUV->release();
    }

    //释放VAO
    if(m_enGraphicsType == enPlane)  //平面
    {
        m_pVAOPlane->release();
    }
    else if(m_enGraphicsType == enCube)  //立方体
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
        LOG(Info, "MyOpenGLWidget::initShaders()---> m_pShaderProgram->link() succ. \n");
    }
    else
    {
        LOG(Warn, "MyOpenGLWidget::initShaders()---> m_pShaderProgram->link() fail. \n");
    }

    m_pShaderProgramYUV = new QOpenGLShaderProgram();
    m_pShaderProgramYUV->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vertexTextureMatrix.vsh");
    m_pShaderProgramYUV->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/textureSampler2D_YUV.fsh");
    bool bRetYUV = m_pShaderProgramYUV->link();
    if(bRetYUV)
    {
        LOG(Info, "MyOpenGLWidget::initShaders()---> m_pShaderProgramYUV->link() succ. \n");
    }
    else
    {
        LOG(Warn, "MyOpenGLWidget::initShaders()---> m_pShaderProgramYUV->link() fail. \n");
    }
}

//立方体
void MyOpenGLWidget::initVertexCube()
{
    GLfloat GLfVertexCube[] = {
        //前面  纹理图片原点在左上角
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  //左下
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  //右下
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  //右上
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  //左上
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  //左下

        //后面
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  //左下
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  //右下
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  //右上
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  //左上
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  //左下

        //左面
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  //后下
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  //前下
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  //前上
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  //前上
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  //后上
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,  //后下

        //右面
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  //前上
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  //后上
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,  //后下
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  //前下
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  //前上

        //底面
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  //左上  纹理原点
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  //右上
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  //右下
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,  //
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

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

    m_pShaderProgram->bind();  //看似操作着色器，其实仅标识VBO中各种属性数据的起始位置，保存到VAO中，绘制时需要再绑定着色器与VAO。
    m_pShaderProgram->setAttributeBuffer("vec2TexurePos", GL_FLOAT, 3*sizeof(GLfloat), 2, (3+2)*sizeof(GLfloat));
    m_pShaderProgram->setAttributeBuffer("vec3VertexPos", GL_FLOAT, 0, 3, (3+2)*sizeof(GLfloat));
    m_pShaderProgram->enableAttributeArray(m_pShaderProgram->attributeLocation("vec3VertexPos"));
    m_pShaderProgram->enableAttributeArray(m_pShaderProgram->attributeLocation("vec2TexurePos"));

    m_iVertexCount = sizeof(GLfVertexCube)/sizeof(GLfVertexCube[0])/5;
    LOG(Info, "MyOpenGLWidget::initVertexCube()---> sizeof(GLfVertexCube) = %d; m_iVertexCount = %d; \n", sizeof(GLfVertexCube), m_iVertexCount);

//    QOpenGLFunctions *pOpenGLFunctions = this->context()->functions();
//    pOpenGLFunctions->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3+2)*sizeof(GLfloat), 0);
//    pOpenGLFunctions->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (3+2)*sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
//    pOpenGLFunctions->glEnableVertexAttribArray(0);
//    pOpenGLFunctions->glEnableVertexAttribArray(1);
}

void MyOpenGLWidget::initVertexPlane()
{
//    GLfloat GLfVertexPlane[] = {
//        -1.0f, -1.0f,  0.0f,  0.0f, 0.0f,  //左下
//         1.0f, -1.0f,  0.0f,  1.0f, 0.0f,  //右下
//         1.0f,  1.0f,  0.0f,  1.0f, 1.0f,  //右上
//        -1.0f,  1.0f,  0.0f,  0.0f, 1.0f   //左上
//    };

    GLfloat GLfVertexPlane[] = {
        //跟上面比，对换纹理图的头脚位置。纹理图片的原点在左上角。
        -1.0f, -1.0f,  0.0f,  0.0f, 1.0f,  //左下
         1.0f, -1.0f,  0.0f,  1.0f, 1.0f,  //右下
         1.0f,  1.0f,  0.0f,  1.0f, 0.0f,  //右上
        -1.0f,  1.0f,  0.0f,  0.0f, 0.0f   //左上
    };

    m_pVAOPlane = new QOpenGLVertexArrayObject();
    m_pVAOPlane->create();
    m_pVAOPlane->bind();

    QOpenGLBuffer *pVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    pVBO->create();
    pVBO->bind();
    pVBO->allocate(GLfVertexPlane, sizeof(GLfVertexPlane));

    m_pShaderProgram->bind();  //看似操作着色器，其实仅标识VBO中各种属性数据的起始位置，保存到VAO中，绘制时需要再绑定着色器与VAO。
    m_pShaderProgram->setAttributeBuffer("vec2TexurePos", GL_FLOAT, 3*sizeof(GLfloat), 2, (3+2)*sizeof(GLfloat));
    m_pShaderProgram->setAttributeBuffer("vec3VertexPos", GL_FLOAT, 0, 3, (3+2)*sizeof(GLfloat));
    m_pShaderProgram->enableAttributeArray(m_pShaderProgram->attributeLocation("vec3VertexPos"));
    m_pShaderProgram->enableAttributeArray(m_pShaderProgram->attributeLocation("vec2TexurePos"));

    m_iVertexCount_Plane = sizeof(GLfVertexPlane)/sizeof(GLfVertexPlane[0])/5;
    LOG(Info, "MyOpenGLWidget::initVertexPlane()---> sizeof(GLfVertexPlane) = %d; m_iVertexCount = %d; \n", sizeof(GLfVertexPlane), m_iVertexCount_Plane);
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
    LOG(Info, "MyOpenGLWidget::initVertexRectRing()---> sizeof(GLfVertexRectRing) = %d; m_iVertexCount = %d; m_iIndexCount = %d; \n", sizeof(GLfVertexRectRing), m_iVertexRectRing, m_iIndexRectRing);
}

//初始化纹理
void MyOpenGLWidget::initTexture()
{
    if(m_imageTexture.load(":/smileface.png"))
    {
        m_pTexture = new QOpenGLTexture(m_imageTexture.mirrored());
        m_enTextureType = enImageTextureCreate;
        //m_bUpdateTexture  = false;
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
    //LOG(Debug, "MyOpenGLWidget::paintGL_TextureProgram( m_enTextureType = %d )... \n", m_enTextureType);

    if(m_enTextureType == enImageTextureCreate || m_enTextureType == enImageTextureUpdate)  //创建图片纹理
    {
        LOG(Info, "MyOpenGLWidget::paintGL_TextureProgram()---> m_enTextureType = enImageTextureCreate[%d]; Create image Texture. \n", m_enTextureType);

        //更新纹理状态
        m_enTextureType = enImageTextureShow;

        //销毁纹理对象
        if(m_pTexture)
        {
            m_pTexture->destroy();
            delete m_pTexture;
            m_pTexture = NULL;
        }

        //创建纹理对象
        m_pTexture = new QOpenGLTexture(m_imageTexture);
        if(m_pTexture)
        {
            //绑定纹理对象
            m_pTexture->bind(0);  //0---30 可改

//            //绑定着色器
//            if(m_pShaderProgram)
//            {
//                m_pShaderProgram->bind();
//                m_pShaderProgram->setUniformValue("texSampler2D", 0);  //--对应绑定的纹理对象
//                m_pShaderProgram->setUniformValue("mat4MVP", m_Matrix4MVP);
//            }
        }
    }
    else if(m_enTextureType == enImageTextureShow)
    {
        //LOG(Debug, "MyOpenGLWidget::paintGL_TextureProgram()---> m_enTextureType = enImageTextureShow[%d]; Show image Texture. \n", m_enTextureType);

        if(m_pTexture)
        {
            //绑定纹理对象
            m_pTexture->bind(0);  //0---30 可改

//            //绑定着色器
//            if(m_pShaderProgram)
//            {
//                m_pShaderProgram->bind();
//                m_pShaderProgram->setUniformValue("texSampler2D", 0);  //--对应绑定的纹理对象
//                m_pShaderProgram->setUniformValue("mat4MVP", m_Matrix4MVP);
//            }
        }
    }
    else if(m_enTextureType == enYUVTextureCreate)  //创建YUV纹理
    {
        LOG(Info, "MyOpenGLWidget::paintGL_TextureProgram()---> m_enTextureType = enYUVTextureCreate[%d]; Create YUV Texture. \n", m_enTextureType);

        //更新纹理状态
        m_enTextureType = enYUVTextureShow;

        m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
        m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1

        //创建纹理对象
        if(m_uTextureId[1] == 0 && m_uTextureId[2] == 0 && m_pYUVFrame)
        {
            glGenTextures(3, m_uTextureId);
            LOG(Info, "MyOpenGLWidget::paintGL_TextureProgram()---> m_uTextureId[0]=%u, m_uTextureId[1]=%u, m_uTextureId[2]=%u; \n", m_uTextureId[0], m_uTextureId[1], m_uTextureId[2]);
        }

        //更新纹理数据
        LOG(Info, "MyOpenGLWidget::paintGL_TextureProgram()---> glTexImage2D(); m_iWidth=%d, m_iHeight=%d; m_iYFrameSize=%d, m_iUFrameSize=%d; \n", m_iWidth, m_iHeight, m_iYFrameSize, m_iUFrameSize);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);  // 为当前绑定的纹理对象设置环绕、过滤方式
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // 环绕方式x方向 重复  //GL_CLAMP_TO_EDGE //GL_CLAMP_TO_BORDER
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // y 方向重复
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // 纹理缩小时的过滤方式 线性
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // 纹理放大时的过滤方式 线性 or GL_NEAREST 相邻
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth, m_iHeight, 0, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);

        //qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> glActiveTexture(GL_TEXTURE1);");
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);

        //qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> glActiveTexture(GL_TEXTURE2);");
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
    }
    else if(m_enTextureType == enYUVTextureUpdate)  //更新YUV纹理
    {
        //LOG(Debug, "MyOpenGLWidget::paintGL_TextureProgram()---> m_enTextureType = enYUVTextureUpdate[%d];  Update YUV Texture. \n", m_enTextureType);

        //更新纹理状态
        m_enTextureType = enYUVTextureShow;

        //更新纹理数据
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth, m_iHeight, 0, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth, m_iHeight, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth/2, m_iHeight/2, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
        //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth/2, m_iHeight/2, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
    }
    else if(m_enTextureType == enYUVTextureShow)  //显示YUV纹理
    {
        //LOG(Debug, "MyOpenGLWidget::paintGL_TextureProgram()---> m_enTextureType = enYUVTextureShow[%d];  Show YUV Texture. \n", m_enTextureType);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
    }
    else
    {
        LOG(Warn, "MyOpenGLWidget::paintGL_TextureProgram()---> Undefine m_enTextureType=%d; \n", m_enTextureType);
    }

    //绑定着色器
    if(enYUVTextureShow == m_enTextureType && m_pShaderProgramYUV)
    {
        m_pShaderProgramYUV->bind();
        m_pShaderProgramYUV->setUniformValue("texSampler2D_Y", 0);  //GL_TEXTURE0
        m_pShaderProgramYUV->setUniformValue("texSampler2D_U", 1);  //GL_TEXTURE1
        m_pShaderProgramYUV->setUniformValue("texSampler2D_V", 2);  //GL_TEXTURE2
        m_pShaderProgramYUV->setUniformValue("mat4MVP", m_Matrix4MVP);
    }
    else if(enImageTextureShow == m_enTextureType && m_pShaderProgram)
    {
        m_pShaderProgram->bind();
        m_pShaderProgram->setUniformValue("texSampler2D", 0);  //--对应绑定的纹理对象
        m_pShaderProgram->setUniformValue("mat4MVP", m_Matrix4MVP);
    }
    else
    {
        LOG(Warn, "MyOpenGLWidget::paintGL_TextureProgram()---> Bind Shader fail! \n");
    }

//    else if(m_enTextureType == enYUVTexture)  //YUV纹理
//    {
//        //创建YUV纹理
//        if(m_uTextureId[1] == 0 && m_uTextureId[2] == 0 && m_pYUVFrame)
//        {
//            m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
//            m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1

//            LOG(Info, "MyOpenGLWidget::paintGL_TextureProgram()---> Create YUV Texture. m_iWidth=%d, m_iHeight=%d; m_iYFrameSize=%d, m_iUFrameSize=%d; \n", m_iWidth, m_iHeight, m_iYFrameSize, m_iUFrameSize);
//            m_bUpdateTextureYUV = false;
//            glGenTextures(3, m_uTextureId);
//            LOG(Info, "MyOpenGLWidget::paintGL_TextureProgram()---> m_uTextureId[0]=%u, m_uTextureId[1]=%u, m_uTextureId[2]=%u; \n", m_uTextureId[0], m_uTextureId[1], m_uTextureId[2]);


//        }
//        else if(m_bUpdateTextureYUV)  //更新YUV纹理
//        {
//            m_iYFrameSize = m_iHeight * m_iWidth;  //计算Y分量的帧尺寸
//            m_iUFrameSize = m_iYFrameSize / 4;     //YUV420P格式: YUV=4:1:1

//            LOG(Debug, "MyOpenGLWidget::paintGL_TextureProgram()---> m_bUpdateTextureYUV = true; m_iWidth=%d, m_iHeight=%d; m_iYFrameSize=%d, m_iUFrameSize=%d; \n", m_iWidth, m_iHeight, m_iYFrameSize, m_iUFrameSize);

//            m_bUpdateTextureYUV = false;

//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);
//            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth, m_iHeight, 0, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);
//            //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth, m_iHeight, GL_RED,GL_UNSIGNED_BYTE, m_pYUVFrame);

//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);
//            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);
//            //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth/2, m_iHeight/2, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize]);

//            glActiveTexture(GL_TEXTURE2);
//            glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
//            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iWidth/2, m_iHeight/2, 0, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
//            //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_iWidth/2, m_iHeight/2, GL_RED,GL_UNSIGNED_BYTE, &m_pYUVFrame[m_iYFrameSize + m_iUFrameSize]);
//        }
//        else  //绑定纹理
//        {
//            //qDebug("MyOpenGLWidget::paintGL_TextureProgram()---> repaint YUV Texture.");

//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, m_uTextureId[0]);

//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, m_uTextureId[1]);

//            glActiveTexture(GL_TEXTURE2);
//            glBindTexture(GL_TEXTURE_2D, m_uTextureId[2]);
//        }
}

void MyOpenGLWidget::drawGraphics()
{
    if(m_enGraphicsType == enPlane)  //平面
    {
        m_pVAOPlane->bind();
        glDrawArrays(GL_TRIANGLE_FAN, 0, m_iVertexCount_Plane);
    }
    else if(m_enGraphicsType == enCube)  //立方体
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
        if(m_enTextureType == enImageTextureUpdate)
        {
            m_pTexture->setData(m_imageTexture);
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

