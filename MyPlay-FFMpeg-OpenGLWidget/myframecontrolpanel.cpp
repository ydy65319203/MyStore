#include "myframecontrolpanel.h"
#include "mylog.h"

CMyFrameControlPanel::CMyFrameControlPanel()
{
    //setAttribute(Qt::WA_TranslucentBackground);
    //this->setWindowOpacity(0.5);

    m_iYUVFrameSize = 0;
    m_iYUVFileSize = 0;
    m_iPlayPos = 0;

    m_pFileYUV = NULL;    //文件指针
    m_pYUVBuffer = NULL;  //YUV数据

    m_iPixelHeigth = 0;
    m_iPixelWidth  = 0;

    m_iYUVTimerId = 0;

    m_iVideoPlayState = enClose;
    m_iAudioPlayState = enClose;

    //--------------------------------------------------------------------------

    m_pMyOpenGLWidget = new MyOpenGLWidget;  //视频
    m_pMyAudioOutput = new CMyAudioOutput;   //音频

    m_pMyFFmpeg = new CMyFFmpeg;  //解码器
    m_pMyFFmpeg->setAudioOutput(m_pMyAudioOutput);
    m_pMyFFmpeg->setVideoOutput(m_pMyOpenGLWidget);

    //控制面板嵌入播放界面
    m_pMyOpenGLWidget->initControlPanel(this);

    connect(m_pMyOpenGLWidget, &MyOpenGLWidget::sig_updatePlayState, this, &CMyFrameControlPanel::OnVideoPlayState);
    connect(m_pMyAudioOutput, &CMyAudioOutput::signal_updatePlayState, this, &CMyFrameControlPanel::OnAudioPlayState);

    //--------------------------------------------------------------------------

    this->setObjectName("ControlPanel");

    //播放按钮
    m_pPushButton_Play = new QPushButton();  //QStringLiteral("播放")
    m_pPushButton_Play->setObjectName("Play");

    //打开按钮
    m_pPushButton_OpenFile = new QPushButton();  //QStringLiteral("打开文件")
    m_pPushButton_OpenFile->setObjectName("Open");

    //显示文件路径
    m_pLineEdit_FilePath   = new QLineEdit(":/smileface.png");
    m_pLineEdit_FilePath->setObjectName("FilePath");
    //m_pLineEdit_FilePath->setClearButtonEnabled(true);

    connect(m_pPushButton_Play, &QPushButton::clicked, this, &CMyFrameControlPanel::OnButton_Play);
    connect(m_pPushButton_OpenFile, &QPushButton::clicked, this, &CMyFrameControlPanel::OnButton_OpenFile);

    m_pPushButton_Cube  = new QPushButton();  //QStringLiteral("立方体")
    m_pPushButton_Ring  = new QPushButton();  //QStringLiteral("梯形环")
    m_pPushButton_Plane = new QPushButton();  //QStringLiteral("平面")
    m_pPushButton_SplitWindow  = new QPushButton();  //QStringLiteral("分割窗口")

    m_pPushButton_Cube->setObjectName("Cube");
    m_pPushButton_Ring->setObjectName("Ring");
    m_pPushButton_Plane->setObjectName("Plane");
    m_pPushButton_SplitWindow->setObjectName("SplitWindow");

    connect(m_pPushButton_SplitWindow, &QPushButton::clicked, m_pMyOpenGLWidget, &MyOpenGLWidget::setQuartering);
    connect(m_pPushButton_Cube,  &QPushButton::clicked, m_pMyOpenGLWidget, &MyOpenGLWidget::setGraphicsTypeCube);
    connect(m_pPushButton_Plane, &QPushButton::clicked, m_pMyOpenGLWidget, &MyOpenGLWidget::setGraphicsTypePlane);
    connect(m_pPushButton_Ring,  &QPushButton::clicked, m_pMyOpenGLWidget, &MyOpenGLWidget::setGraphicsTypeRectRing);

    //connect(m_pPushButton_Plane, &QPushButton::clicked, this, &CMyFrameControlPanel::startAudio);
    //connect(m_pPushButton_Plane, &QPushButton::clicked, m_pMyAudioOutput, &CMyAudioOutput::OnStopAudioOutput);

    //打开按钮横向布局
    QHBoxLayout *pHBoxLayoutOpenFile = new QHBoxLayout;
    //pHBoxLayoutPlayButton->addWidget(m_pPushButton_Play);
    pHBoxLayoutOpenFile->addSpacing(10);
    pHBoxLayoutOpenFile->addWidget(m_pLineEdit_FilePath);
    pHBoxLayoutOpenFile->addWidget(m_pPushButton_OpenFile);
    pHBoxLayoutOpenFile->addStretch();
    //pHBoxLayoutOpenFile->addSpacing(50);
    pHBoxLayoutOpenFile->addWidget(m_pPushButton_Plane);
    pHBoxLayoutOpenFile->addWidget(m_pPushButton_Cube);
    pHBoxLayoutOpenFile->addWidget(m_pPushButton_Ring);
    pHBoxLayoutOpenFile->addWidget(m_pPushButton_SplitWindow);

    //--------------------------------------------------------------------------

    m_pSliderPlay = new QSlider(Qt::Horizontal);
    m_pSliderPlay->setValue(0);
    m_pSliderPlay->setMinimum(0);
    m_pSliderPlay->setMaximum(3600);
    m_pSliderPlay->setSingleStep(1);
    m_pSliderPlay->setObjectName("Play");
    //m_pSliderPlay->setInvertedControls(true);  //滑动方向反转
    //m_pSliderPlay->setInvertedAppearance(true);
    m_pSliderPlay->setTracking(false);  //鼠标抬起时才发出ChangeValue信号

    m_pSliderX = new QSlider(Qt::Horizontal);
    m_pSliderX->setValue(0);
    m_pSliderX->setMaximum(180);
    m_pSliderX->setMinimum(-180);
    m_pSliderX->setSingleStep(5);
    m_pSliderX->setObjectName("X");

    m_pSliderY = new QSlider(Qt::Horizontal);
    m_pSliderY->setValue(0);
    m_pSliderY->setMaximum(180);
    m_pSliderY->setMinimum(-180);
    m_pSliderY->setSingleStep(5);
    m_pSliderY->setObjectName("Y");

    m_pSliderZ = new QSlider(Qt::Horizontal);
    m_pSliderZ->setValue(0);
    m_pSliderZ->setMaximum(180);
    m_pSliderZ->setMinimum(-180);
    m_pSliderZ->setSingleStep(5);
    m_pSliderZ->setObjectName("Z");

    connect(m_pSliderX, &QSlider::valueChanged, m_pMyOpenGLWidget, &MyOpenGLWidget::rotateX);
    connect(m_pSliderY, &QSlider::valueChanged, m_pMyOpenGLWidget, &MyOpenGLWidget::rotateY);
    connect(m_pSliderZ, &QSlider::valueChanged, m_pMyOpenGLWidget, &MyOpenGLWidget::rotateZ);

    //XYZ旋转滑块横向布局
    QHBoxLayout *pHBoxLayoutXYZ = new QHBoxLayout;
    pHBoxLayoutXYZ->addSpacing(10);
    pHBoxLayoutXYZ->addWidget(m_pSliderX);
    pHBoxLayoutXYZ->addWidget(m_pSliderY);
    pHBoxLayoutXYZ->addWidget(m_pSliderZ);

    //--------------------------------------------------------------------------

    //组合打开文件行和XYZ行
    QVBoxLayout *pVBoxLayoutOpenFileXYZ = new QVBoxLayout;
    pVBoxLayoutOpenFileXYZ->addLayout(pHBoxLayoutXYZ);
    pVBoxLayoutOpenFileXYZ->addLayout(pHBoxLayoutOpenFile);

    //播放按钮横向布局
    QHBoxLayout *pHBoxLayoutPlay = new QHBoxLayout;
    pHBoxLayoutPlay->addWidget(m_pPushButton_Play);
    pHBoxLayoutPlay->addLayout(pVBoxLayoutOpenFileXYZ);

    //界面主布局
    QVBoxLayout *pVBoxLayoutMain = new QVBoxLayout;
    pVBoxLayoutMain->addStretch();
    pVBoxLayoutMain->addWidget(m_pSliderPlay);
    pVBoxLayoutMain->addLayout(pHBoxLayoutPlay);

    this->setLayout(pVBoxLayoutMain);
}

CMyFrameControlPanel::~CMyFrameControlPanel()
{
    LOG(Info, "CMyFrameControlPanel::~CMyFrameControlPanel()... \n");

    if (m_pMyFFmpeg)
    {
        LOG(Info, "CMyFrameControlPanel::~CMyFrameControlPanel()---> m_pMyFFmpeg->closeAVFile(); \n");
        m_pMyFFmpeg->closeAVFile();
    }

    if (m_pMyAudioOutput)
    {
        LOG(Info, "CMyFrameControlPanel::~CMyFrameControlPanel()---> m_pMyAudioOutput->OnStopAudioOutput(); \n");
        m_pMyAudioOutput->OnStopAudioOutput();
    }
}

MyOpenGLWidget *CMyFrameControlPanel::getMyOpenGLWidget()
{
    return m_pMyOpenGLWidget;
}

void CMyFrameControlPanel::OnVideoPlayState(int iState)
{
    LOG(Info, "CMyFrameControlPanel::OnVideoPlayState( iState=%d )... \n", iState);

    m_iVideoPlayState = iState;

    if (m_pPushButton_Play)
    {
        switch (m_iVideoPlayState)
        {
        case enClose:
            LOG(Info, "CMyFrameControlPanel::OnVideoPlayState()---> m_pPushButton_Play->setStyleSheet(NULL), setEnabled(false); \n");
            m_pPushButton_Play->setStyleSheet("");
            m_pPushButton_Play->setEnabled(false);
            break;

        case enOpen:
            LOG(Info, "CMyFrameControlPanel::OnVideoPlayState()---> m_pPushButton_Play->setStyleSheet(NULL), setEnabled(true); \n");
            m_pPushButton_Play->setStyleSheet("");
            m_pPushButton_Play->setEnabled(true);
            break;

        case enPlay:
            //播放时，显示暂停按钮。
            LOG(Info, "CMyFrameControlPanel::OnVideoPlayState()---> m_pPushButton_Play->setStyleSheet( border-image: pause.png ); \n");
            m_pPushButton_Play->setStyleSheet("border-image: url(D:/YDY/SourceCode/MyFFmpeg/MyPicture/pause.png);");
            break;

        case enPause:
            //暂停时，显示播放按钮。
            LOG(Info, "CMyFrameControlPanel::OnVideoPlayState()---> m_pPushButton_Play->setStyleSheet(NULL); \n");
            m_pPushButton_Play->setStyleSheet("");
            break;

        default:
            break;
        }
    }
}

void CMyFrameControlPanel::OnAudioPlayState(int iState)
{
    LOG(Info, "CMyFrameControlPanel::OnAudioPlayState( iState=%d )... \n", iState);

    m_iAudioPlayState = iState;

    if (m_pPushButton_Play)
    {
        switch (m_iAudioPlayState)
        {
        case enClose:
            LOG(Info, "CMyFrameControlPanel::OnAudioPlayState()---> m_pPushButton_Play->setStyleSheet(NULL), setEnabled(false); \n");
            m_pPushButton_Play->setStyleSheet("");
            m_pPushButton_Play->setEnabled(false);
            break;

        case enOpen:
            LOG(Info, "CMyFrameControlPanel::OnAudioPlayState()---> m_pPushButton_Play->setStyleSheet(NULL), setEnabled(true); \n");
            m_pPushButton_Play->setStyleSheet("");
            m_pPushButton_Play->setEnabled(true);
            break;

        case enPlay:
            //播放时，显示暂停按钮。
            LOG(Info, "CMyFrameControlPanel::OnAudioPlayState()---> m_pPushButton_Play->setStyleSheet(border-image: pause.png); \n");
            m_pPushButton_Play->setStyleSheet("border-image: url(D:/YDY/SourceCode/MyFFmpeg/MyPicture/pause.png);");
            break;

        case enPause:
            //暂停时，显示播放按钮。
            LOG(Info, "CMyFrameControlPanel::OnAudioPlayState()---> m_pPushButton_Play->setStyleSheet(NULL); \n");
            m_pPushButton_Play->setStyleSheet("");
            break;

        default:
            break;
        }
    }
}

void CMyFrameControlPanel::OnButton_OpenFile()
{
    LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()... \n");

    //取文件名  D:\YDY\SourceCode\MyFFmpeg\MyFilm
    m_qstrImageFile = QFileDialog::getOpenFileName(this, QStringLiteral("选择纹理图片"), "D:\\YDY\\SourceCode\\MyFFmpeg\\MyFilm");
    if (m_qstrImageFile.isEmpty())
    {
        LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> QFileDialog::getOpenFileName() = NULL; return; \n");
        return;
    }

    std::string sstrImageFile = m_qstrImageFile.toStdString();
    LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> QFileDialog::getOpenFileName() = %s \n", sstrImageFile.c_str());

    //显示文件名
    m_pLineEdit_FilePath->setText(m_qstrImageFile);

    //杀定时器
    if(m_iYUVTimerId > 0)
    {
        killTimer(m_iYUVTimerId);
        m_iYUVTimerId = 0;
    }

    //关闭旧文件
    if(m_pYUVBuffer && m_pFileYUV)
    {
        LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> m_pFileYUV->close(); \n");
        m_pFileYUV->unmap(m_pYUVBuffer);
        m_pFileYUV->close();
        delete m_pFileYUV;
        m_pFileYUV = NULL;
        m_iPlayPos = 0;
        m_pYUVBuffer = NULL;
    }

    if (m_pMyFFmpeg)
    {
        LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> m_pMyFFmpeg->closeAVFile(); \n");
        m_pMyFFmpeg->closeAVFile();
    }

    //更新视频状态
    if (m_iVideoPlayState != enClose)
    {
        LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnVideoPlayState(enClose=%d); \n", enClose);
        this->OnVideoPlayState(enClose);
    }

    //更新音频状态
    if (m_iAudioPlayState != enClose)
    {
        LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnAudioPlayState(enClose=%d); \n", enClose);
        this->OnAudioPlayState(enClose);
    }

    //--------------------------------------------------

    //检查文件大小
    QFileInfo fileInfo(m_qstrImageFile);
    m_iYUVFileSize = fileInfo.size();
    if (m_iYUVFileSize > (1024 * 1024 * 1024))
    {
        LOG(Warn, "CMyFrameControlPanel::OnButton_OpenFile()---> The file is too big! m_iFileSize = %d; \n", m_iYUVFileSize);
        return;
    }

    //检查文件扩展名
    m_qstrFileSuffix = fileInfo.suffix().toLower();
    if (m_imageTexture.load(m_qstrImageFile))  //if (m_qstrFileSuffix == "png" || m_qstrFileSuffix == "jpg")
    {
        //更新纹理
        LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> m_pMyOpenGLWidget->setImageTexture(m_imageTexture); \n");
        m_pMyOpenGLWidget->setImageTexture(m_imageTexture);
    }
    else if (m_qstrFileSuffix == "yuv")
    {
        //YUV视频
        LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> openYUVFile(); \n");
        openYUVFile(m_qstrImageFile);
    }
    else if(m_pMyFFmpeg)
    {
        //LOG(Info, "CMyFrameControlPanel::selectImageFile()---> Unknown type: %s \n", fileInfo.fileName().toStdString().c_str());
        LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile()---> m_myFFmpeg.openAVFile(); \n");
        m_pMyFFmpeg->openAVFile(sstrImageFile);
    }
    else
    {
        LOG(Warn, "CMyFrameControlPanel::OnButton_OpenFile()---> Open file fail: %s \n", sstrImageFile.c_str());
    }

    //LOG(Info, "CMyFrameControlPanel::OnButton_OpenFile() End \n");
}

void CMyFrameControlPanel::OnButton_Play()
{
    LOG(Info, "CMyFrameControlPanel::OnButton_Play()... \n");

    LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> m_iVideoPlayState=%d; m_iAudioPlayState=%d; \n", m_iVideoPlayState, m_iAudioPlayState);

    if (m_iVideoPlayState == enPause || m_iAudioPlayState == enPause)
    {
        //取消暂停
        LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> m_pMyFFmpeg->Pause(); \n");
        m_pMyFFmpeg->Pause();

        //更新视频状态
        if (m_iVideoPlayState == enPause)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnVideoPlayState(enPlay=%d); \n", enPlay);
            this->OnVideoPlayState(enPlay);
        }

        //更新音频状态
        if (m_iAudioPlayState == enPause)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnAudioPlayState(enPlay=%d); \n", enPlay);
            this->OnAudioPlayState(enPlay);
        }
    }
    else if (m_iVideoPlayState == enPlay || m_iAudioPlayState == enPlay)
    {
        LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> m_pMyFFmpeg->Pause(); \n");
        m_pMyFFmpeg->Pause();

        //更新视频状态
        if (m_iVideoPlayState == enPlay)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnVideoPlayState(enPause=%d); \n", enPause);
            this->OnVideoPlayState(enPause);
        }

        //更新音频状态
        if (m_iAudioPlayState == enPlay)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnAudioPlayState(enPause=%d); \n", enPause);
            this->OnAudioPlayState(enPause);
        }
    }
    else if (m_iVideoPlayState == enOpen || m_iAudioPlayState == enOpen)
    {
        //启动播放
        if (m_qstrFileSuffix == "yuv")
        {
            //启动定时器
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> startTimer(40) = %d; \n", m_iYUVTimerId);
            m_iYUVTimerId = this->startTimer(40);  //Qt::PreciseTimer精度计时 
        }
        else if (m_pMyFFmpeg)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> m_pMyFFmpeg->Play(); \n");
            m_pMyFFmpeg->Play();
        }

        //------------------------------------------------------------------------------------

        //更新视频状态
        if (m_iVideoPlayState == enOpen)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnVideoPlayState(enPlay=%d); \n", enPlay);
            this->OnVideoPlayState(enPlay);
        }

        //更新音频状态
        if (m_iAudioPlayState == enOpen)
        {
            LOG(Info, "CMyFrameControlPanel::OnButton_Play()---> OnAudioPlayState(enPlay=%d); \n", enPlay);
            this->OnAudioPlayState(enPlay);
        }
    }

}

//响应定时器事件
void CMyFrameControlPanel::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_iYUVTimerId)
    {
        playYUVFrame();
    }
}

void CMyFrameControlPanel::openYUVFile(QString &qstrFileName)
{
    LOG(Info, "CMyFrameControlPanel::openYUVFile()... \n");

    //从文件名解析出帧尺寸：test_yuv420p_320x180.yuv
    //截取480x272
    QString qstrPixelSize = qstrFileName.mid(qstrFileName.lastIndexOf("_")+1);
    qstrPixelSize.truncate(qstrPixelSize.lastIndexOf(".yuv"));
    qstrPixelSize = qstrPixelSize.toLower();  //转成小写

    QString qstrWidth = qstrPixelSize.left(qstrPixelSize.indexOf("x"));
    QString qstrHeight = qstrPixelSize.mid(qstrPixelSize.indexOf("x")+1);

    //转为int
    m_iPixelWidth = qstrWidth.toUInt();
    m_iPixelHeigth = qstrHeight.toUInt();

    //计算帧尺寸YUV420Plane
    m_iYUVFrameSize = m_iPixelWidth * m_iPixelHeigth * 1.5;
    LOG(Info, "CMyFrameControlPanel::openYUVFile()---> m_iYUVFileSize=%d; m_iYUVFrameSize=%d, m_iPixelWidth=%d, m_iPixelHeigth=%d; ", m_iYUVFileSize, m_iYUVFrameSize, m_iPixelWidth, m_iPixelHeigth);

    //关闭旧文件
    if(m_pYUVBuffer && m_pFileYUV)
    {
        m_pFileYUV->unmap(m_pYUVBuffer);
        m_pFileYUV->close();
        delete m_pFileYUV;
        m_pFileYUV = NULL;
        m_iPlayPos = 0;
        m_pYUVBuffer = NULL;
    }

    //映射新文件
    m_pFileYUV = new QFile(qstrFileName);  //m_pFileYUV->setFileName(qstrFileName);
    if(m_pFileYUV->open(QIODevice::ReadOnly))
    {
        //映射文件到内存
        m_pYUVBuffer = m_pFileYUV->map(0, m_iYUVFileSize);

        //播放第一帧
        //playYUVFrame();
        m_pMyOpenGLWidget->setYUVTexture(m_pYUVBuffer, m_iPixelWidth, m_iPixelHeigth);
        m_iPlayPos += m_iYUVFrameSize;

        //启用Play按钮
        //m_pPushButton_Play->setEnabled(true);

        //更新播放状态
        LOG(Info, "CMyFrameControlPanel::openYUVFile()---> OnVideoPlayState(enOpen=%d); \n", enOpen);
        this->OnVideoPlayState(enOpen);
    }
    else
    {
        LOG(Warn, "CMyFrameControlPanel::openYUVFile()---> Open file fail: %s \n", qstrFileName.toStdString().c_str());
    }
}

//ds_480x272.yuv
void CMyFrameControlPanel::parseYUVFileName(QString &qstrFileName, int &iWidth, int &iHeight)
{
    //截取480x272
    QString qstrPixelSize = qstrFileName.mid(qstrFileName.lastIndexOf("_")+1);
    qstrPixelSize.truncate(qstrPixelSize.lastIndexOf(".yuv"));
    qstrPixelSize = qstrPixelSize.toLower();  //转成小写

    QString qstrWidth = qstrPixelSize.left(qstrPixelSize.indexOf("x"));
    QString qstrHeight = qstrPixelSize.mid(qstrPixelSize.indexOf("x")+1);

    //转为int
    iWidth = qstrWidth.toUInt();
    iHeight = qstrHeight.toUInt();

    //计算帧尺寸YUV420Plane
    m_iYUVFrameSize = iWidth * iHeight * 1.5;
}

void CMyFrameControlPanel::playYUVFrame()
{
    //qDebug("MainWindow::playYUVFrame()...");
    if((m_iPlayPos + m_iYUVFrameSize) > m_iYUVFileSize)
    {
        m_iPlayPos = 0;
    }

    m_pMyOpenGLWidget->updateYUVTexture(m_pYUVBuffer + m_iPlayPos);
    m_iPlayPos += m_iYUVFrameSize;
}


void CMyFrameControlPanel::startAudio()
{
    //设置音频缓存区
    //LOG(Info, "CMyFrameControlPanel::startAudio()---> m_pMyAudioOutput->setFrameBuffer(iFrameData=%d, iFrameCount=%d); \n", iAudioFrameData, iAudioFrameCount);
    int iRet = m_pMyAudioOutput->setAudioBuffer(4096, 16);  //iFrameCount=16;
    if (iRet < 0)
    {
        LOG(Error, "CMyFrameControlPanel::startAudio()---> m_pMyAudioOutput->setFrameBuffer() = %d; break; \n", iRet);
    }

    //设置音频播放格式
    //LOG(Info, "CMyFrameControlPanel::startAudio()---> m_pMyAudioOutput->setAudioFormat(iChannel=2, iSampleRate=%d, iSampleFormat=%d); \n", pAVFrame->sample_rate, pAVFrame->format);
    iRet = m_pMyAudioOutput->setAudioFormat(2, 44100, 0);
    if (iRet < 0)
    {
        LOG(Error, "CMyFrameControlPanel::startAudio()---> m_pMyAudioOutput->setAudioFormat() = %d; break; \n", iRet);
    }

    //启动音频播放
    LOG(Info, "CMyFrameControlPanel::startAudio()---> m_pMyAudioOutput->OnStartAudioOutput(); \n");
    m_pMyAudioOutput->OnStartAudioOutput();
}
