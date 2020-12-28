﻿#include "myaudio.h"
#include "mylog.h"

enum PlayState
{
    enClose = 0,
    enOpen,
    enPlay,
    enPause,
};

CMyAudioBuffer::CMyAudioBuffer()
{
    m_pFrameBuffer = NULL;
    m_iFrameBuffer = 0;
    m_iFrameCount  = 0;
    m_iFrameSize   = 0;

    m_pWorkFrame = NULL;
    m_iBufferDuration = 0;
//     m_iDuration = 0;
//     m_iPTS = 0;
//     m_iDTS = 0;
}

CMyAudioBuffer::~CMyAudioBuffer()
{
    LOG(Info, "CMyAudioBuffer::~CMyAudioBuffer()... \n");
    clearAudioBuffer();
}

int CMyAudioBuffer::setAudioBuffer(int iFrameData, int iFrameCount)
{
    LOG(Info, "CMyAudioBuffer::setAudioBuffer(iFrameData=%d, iFrameCount=%d)... \n", iFrameData, iFrameCount);

    //参数校验
    if(iFrameData <= 0 || (MByte*10) < iFrameData || iFrameCount <= 0 || KByte < iFrameCount)
    {
        LOG(Error, "CMyAudioBuffer::setAudioBuffer()---> Value out of range: iFrameData, iFrameCount; \n");
        return -1;
    }

    //清空Free列表
    m_MutexFree.lock();  //--加锁
    m_listFree.clear();
    m_MutexFree.unlock();  //--解锁

    //清空Work列表
    m_MutexWork.lock();  //--加锁
    m_listWork.clear();
    m_iBufferDuration = 0;
    m_pWorkFrame = NULL;
//     m_iDuration = 0;
//     m_iPTS = 0;
//     m_iDTS = 0;
    m_MutexWork.unlock();  //--解锁

    //释放缓存
    if(m_pFrameBuffer)
    {
        delete[] m_pFrameBuffer;
        m_pFrameBuffer = NULL;
        m_iFrameBuffer = 0;
        m_iFrameCount = 0;
        m_iFrameSize = 0;
        m_iFrameData = 0;
    }

    //-----------------------------------------------------------------

    //重新赋值
    m_iFrameData = iFrameData;
    m_iFrameCount = iFrameCount;

    //分配空间
    m_iFrameSize = m_iFrameData + sizeof(CMyFrame);
    m_iFrameBuffer = m_iFrameSize * m_iFrameCount;
    m_pFrameBuffer = new uint8_t [m_iFrameBuffer];
    if(m_pFrameBuffer == NULL)
    {
        LOG(Error, "CMyAudioBuffer::setAudioBuffer()---> new m_pFrameBuffer[%d] = NULL; \n", m_iFrameBuffer);
        return -2;
    }

    //预置链表
    m_MutexFree.lock();  //--加锁
    m_listFree.clear();
    uint8_t *pBegin = m_pFrameBuffer;
    uint8_t *pEnd = m_pFrameBuffer + m_iFrameBuffer;
    for(; pBegin < pEnd; pBegin += m_iFrameSize)
    {
        m_listFree.push_back((CMyFrame*)pBegin);
    }
    size_t iFreeCount = m_listFree.size();
    m_MutexFree.unlock();  //--解锁

    m_MutexWork.lock();    //--加锁
    m_listWork.clear();
    size_t iWorkCount = m_listWork.size();
    m_MutexWork.unlock();  //--解锁

    LOG(Info, "CMyAudioBuffer::setAudioBuffer()---> m_listFree.size()=%d; m_listWork.size()=%d; m_iFrameSize=%d; m_iFrameBuffer=%d; \n", iFreeCount, iWorkCount, m_iFrameSize, m_iFrameBuffer);

    return true;
}

void CMyAudioBuffer::clearAudioBuffer()
{
    LOG(Info, "CMyAudioBuffer::clearAudioBuffer()... \n");

    //清空Free列表
    m_MutexFree.lock();  //--加锁
    m_listFree.clear();
    m_MutexFree.unlock();  //--解锁

    //清空Work列表
    m_MutexWork.lock();  //--加锁
    m_listWork.clear();
    m_iBufferDuration = 0;
    m_pWorkFrame = NULL;
    m_MutexWork.unlock();  //--解锁

    //释放缓存
    if (m_pFrameBuffer)
    {
        delete[] m_pFrameBuffer;
        m_pFrameBuffer = NULL;
        m_iFrameBuffer = 0;
        m_iFrameCount  = 0;
        m_iFrameSize = 0;
        m_iFrameData = 0;
    }
}

void CMyAudioBuffer::pushFree(CMyFrame *pFrame)
{
    if(pFrame)
    {
        m_MutexFree.lock();  //--加锁
        m_listFree.push_back(pFrame);
        m_MutexFree.unlock();  //--解锁
    }
}

void CMyAudioBuffer::pushWork(CMyFrame *pFrame)
{
    if(pFrame)
    {
        m_MutexWork.lock();  //--加锁
        m_listWork.push_back(pFrame);
        m_iBufferDuration += pFrame->duration;
        m_MutexWork.unlock();  //--解锁
    }
}

CMyFrame *CMyAudioBuffer::popFree()
{
    CMyFrame *pFrame = NULL;

    m_MutexFree.lock();  //--加锁
    if(m_listFree.size() > 0)
    {
        pFrame = m_listFree.front();
        m_listFree.pop_front();
    }
    m_MutexFree.unlock();  //--解锁

    return pFrame;
}

CMyFrame *CMyAudioBuffer::popWork()
{
    CMyFrame *pFrame = NULL;

    m_MutexWork.lock();  //--加锁
    if(m_listWork.size() > 0)
    {
        pFrame = m_listWork.front();
        m_iBufferDuration -= pFrame->duration;
        m_listWork.pop_front();
    }
    m_MutexWork.unlock();  //--解锁

    return pFrame;
}

CMyFrame *CMyAudioBuffer::getWork()
{
    if(m_pWorkFrame)
    {
        m_MutexFree.lock();  //--加锁
        m_listFree.push_back(m_pWorkFrame);
        m_MutexFree.unlock();  //--解锁
    }

    m_MutexWork.lock();  //--加锁
    if(m_listWork.size() > 0)
    {
        m_pWorkFrame = m_listWork.front();
        m_iBufferDuration -= m_pWorkFrame->duration;
        m_listWork.pop_front();
    }
    else
    {
        m_pWorkFrame = NULL;
    }
    m_MutexWork.unlock();  //--解锁

    return m_pWorkFrame;
}

// int64_t CMyAudioBuffer::getPTS()
// {
//     return m_iPTS;
// }
// 
// int64_t CMyAudioBuffer::getDTS()
// {
//     return m_iDTS;
// }
// 
// int64_t CMyAudioBuffer::getDuration()
// {
//     return m_iDuration;
// }

int CMyAudioBuffer::getFrameData()
{
    return m_iFrameData;
}

//=======================================================================================

CMyAudioOutput::CMyAudioOutput()
{
    //连接本对象的信号槽，以支持其他线程调用。
    connect(this, &CMyAudioOutput::signal_startAudioOutput, this, &CMyAudioOutput::OnStartAudioOutput);
    connect(this, &CMyAudioOutput::signal_pauseAudioOutput, this, &CMyAudioOutput::OnPauseAudioOutput);
    connect(this, &CMyAudioOutput::signal_stopAudioOutput, this, &CMyAudioOutput::OnStopAudioOutput);

//     m_audioFormat.setSampleRate(44100);  //44100
//     m_audioFormat.setChannelCount(2);    //AV_CH_LAYOUT_STEREO = 2
//     m_audioFormat.setCodec("audio/pcm");
//     m_audioFormat.setSampleSize(16);     //iSampleFormat: AV_SAMPLE_FMT_S16 = 1
//     m_audioFormat.setSampleType(QAudioFormat::SignedInt);
//     m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);

    m_pAudioOutput = NULL;

    m_pFrame = NULL;
    m_pData  = NULL;
    m_iData  = 0;

    m_iDuration = 0;
    m_iPTS = 0;
    m_iDTS = 0;

    m_iVolume = 0;  //音量

    m_enState = QAudio::StoppedState;

    m_bSetAudioFormat = false;
}

CMyAudioOutput::~CMyAudioOutput()
{
    this->OnStopAudioOutput();
}

void CMyAudioOutput::startAudioOutput()
{
    LOG(Info, "CMyAudioOutput::startAudioOutput()---> emit signal_startAudioOutput(); \n");
    emit signal_startAudioOutput();
}

void CMyAudioOutput::pauseAudioOutput()
{
    LOG(Info, "CMyAudioOutput::pauseAudioOutput()---> emit signal_pauseAudioOutput(); \n");
    emit signal_pauseAudioOutput();
}

void CMyAudioOutput::stopAudioOutput()
{
    LOG(Info, "CMyAudioOutput::stopAudioOutput()---> emit signal_stopAudioOutput(); \n");
    emit signal_stopAudioOutput();
}

void CMyAudioOutput::updatePlayState(int iState)
{
    LOG(Info, "CMyAudioOutput::updatePlayState()---> emit sig_updatePlayState(iState=%d); \n", iState);
    emit signal_updatePlayState(iState);
}

int CMyAudioOutput::setAudioFormat(int iChannel, int iSampleRate, int iSampleFormat)
{
    LOG(Info, "CMyAudioOutput::setAudioFormat(iChannel=%d, iSampleRate=%d, iSampleFormat=%d)... \n", iChannel, iSampleRate, iSampleFormat);

    if (m_enState != QAudio::StoppedState)
    {
        LOG(Warn, "CMyAudioOutput::setAudioFormat()---> (m_enState = %d) != QAudio::StoppedState; \n", m_enState);
        return -1;
    }

    m_bSetAudioFormat = false;

    m_audioFormat.setSampleRate(iSampleRate);  //44100
    m_audioFormat.setChannelCount(iChannel);   //AV_CH_LAYOUT_STEREO = 2
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setSampleSize(16);           //iSampleFormat: AV_SAMPLE_FMT_S16 = 1
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);

    if(!m_audioFormat.isValid())
    {
        LOG(Warn, "CMyAudioOutput::setAudioFormat()---> m_audioFormat.isValid() = false; \n");
        return -1;
    }

    //-----------------------------------------------------------------

    //检查输出设备
    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    if(defaultDeviceInfo.isNull())
    {
        LOG(Warn, "CMyAudioOutput::setAudioFormat()---> defaultDeviceInfo.isNull(); \n");
        return -1;
    }

    //支持的音频格式
    if(!defaultDeviceInfo.isFormatSupported(m_audioFormat))
    {
        QAudioFormat audioFormat = defaultDeviceInfo.nearestFormat(m_audioFormat);
        LOG(Warn, "CMyAudioOutput::setAudioFormat()---> defaultDeviceInfo.isFormatSupported(m_audioFormat) = false; \n");
        LOG(Warn, "CMyAudioOutput::setAudioFormat()---> defaultDeviceInfo.nearestFormat(m_audioFormat): sampleRate()=%d; sampleSize()=%d; sampleType()=%d; \n", audioFormat.sampleRate(), audioFormat.sampleSize(), audioFormat.sampleType());
        return -2;
    }

    LOG(Info, "CMyAudioOutput::setAudioFormat()---> defaultDeviceInfo.deviceName() = %s \n", defaultDeviceInfo.deviceName().toStdString().c_str());

    //设备名
//     for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
//     {
//         if (deviceInfo != defaultDeviceInfo)
//         {
//             LOG(Info, "CMyAudioOutput::setAudioFormat()---> AudioOutput.deviceName() = %s \n", deviceInfo.deviceName().toStdString().c_str());
//         }
//     }

    //设备音频格式成功
    LOG(Info, "CMyAudioOutput::setAudioFormat()---> Set m_bSetAudioFormat = true; \n");
    m_bSetAudioFormat = true;
    return true;
}

//int CMyAudioOutput::setFrameBuffer(CMyAudioBuffer *pMyFrameBuffer)
//{
//    LOG(Info, "CMyAudioOutput::setFrameBuffer(pMyFrameBuffer = %p)... \n", pMyFrameBuffer);
//    m_pMyAudioData = pMyFrameBuffer;
//    return true;
//}

int CMyAudioOutput::setVolume(int iValue)
{
    LOG(Info, "CMyAudioOutput::setVolume( iValue=%d )... \n", iValue);

    qreal linearVolume = QAudio::convertVolume(iValue / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    if(m_pAudioOutput)
    {
        LOG(Info, "CMyAudioOutput::setVolume()---> m_pAudioOutput->setVolume(linearVolume=%lf); \n", linearVolume);
        m_pAudioOutput->setVolume(linearVolume);
        m_iVolume = iValue;
    }

    return m_iVolume;
}

void CMyAudioOutput::OnStartAudioOutput()
{
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()... \n");

    if(m_pFrameBuffer == NULL)
    {
        LOG(Warn, "CMyAudioOutput::OnStartAudioOutput()---> m_pFrameBuffer = NUL; \n");
        return;
    }

    if(!m_bSetAudioFormat)
    {
        LOG(Warn, "CMyAudioOutput::OnStartAudioOutput()---> setAudioFormat() = false; \n");
        return;
    }

    if (m_enState != QAudio::StoppedState)
    {
        LOG(Warn, "CMyAudioOutput::OnStartAudioOutput()---> (m_enState = %d) != QAudio::StoppedState; \n", m_enState);
        return;
    }

    m_pFrame = NULL;
    m_pData  = NULL;
    m_iData  = 0;

    //-----------------------------------------------------------------

    //打开设备
    if(!QIODevice::open(QIODevice::ReadOnly))
    {
        LOG(Error, "CMyAudioOutput::OnStartAudioOutput()---> QIODevice::open(QIODevice::ReadOnly) = false; \n");
        return;
    }

    //创建播放器
    m_pAudioOutput = new QAudioOutput(m_audioFormat);
    if(m_pAudioOutput == NULL)
    {
        LOG(Error, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput = new QAudioOutput(m_audioFormat) is NULL! \n");
        return;
    }

    //取缺省缓存区大小
    int iBufferSize = m_pAudioOutput->bufferSize();
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->bufferSize() = %d; \n", iBufferSize);

    //设置缓存大小
    //LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->setBufferSize(m_iFrameData=%d); \n", m_iFrameData);
    //m_pAudioOutput->setBufferSize(m_iFrameData);

    //启动播放
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->start(this); \n");
    m_pAudioOutput->start(this);

    iBufferSize = m_pAudioOutput->bufferSize();
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->bufferSize() = %d; \n", iBufferSize);

    //取播放状态
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->state(); \n");
    m_enState = m_pAudioOutput->state();
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->state() = %d; \n", m_enState);

    //上报状态
    if (m_enState == QAudio::ActiveState)
    {
        LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> emit signal_updatePlayState( enPlay = %d ); \n", enPlay);
        emit signal_updatePlayState(enPlay);
    }

    //取音量
    qreal initialVolume = QAudio::convertVolume(m_pAudioOutput->volume(),
                                                QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_iVolume = qRound(initialVolume * 100);
    LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> m_pAudioOutput->volume() = %d; \n", m_iVolume);

    LOG(Info, "CMyAudioOutput::OnStartAudioOutput() End \n");
    return;
}


void CMyAudioOutput::OnPauseAudioOutput()
{
    LOG(Info, "CMyAudioOutput::OnPauseAudioOutput()... \n");

    if (m_pAudioOutput->state() == QAudio::SuspendedState || m_pAudioOutput->state() == QAudio::StoppedState)
    {
        LOG(Info, "CMyAudioOutput::OnPauseAudioOutput()---> m_pAudioOutput->resume(); \n");
        m_pAudioOutput->resume();
        //m_suspendResumeButton->setText(tr("Suspend recording"));
    }
    else if (m_pAudioOutput->state() == QAudio::ActiveState)
    {
        LOG(Info, "CMyAudioOutput::OnPauseAudioOutput()---> m_pAudioOutput->suspend(); \n");
        m_pAudioOutput->suspend();
        //m_suspendResumeButton->setText(tr("Resume playback"));
    }
    else if (m_pAudioOutput->state() == QAudio::IdleState)
    {
        // no-op
    }

    m_enState = m_pAudioOutput->state();
    LOG(Info, "CMyAudioOutput::OnPauseAudioOutput()---> m_pAudioOutput->state() = %d; \n", m_enState);

    return;
}


void CMyAudioOutput::OnStopAudioOutput()
{
    LOG(Info, "CMyAudioOutput::OnStopAudioOutput()... \n");

    //关闭设备
    QIODevice::close();

    //停止播放
    if(m_pAudioOutput)
    {
        LOG(Info, "CMyAudioOutput::OnStopAudioOutput()---> m_pAudioOutput->stop(); \n");
        m_pAudioOutput->stop();
        //delete m_pAudioOutput;
        //m_pAudioOutput = NULL;
    }

    m_pFrame = NULL;
    m_pData  = NULL;
    m_iData  = 0;

    LOG(Info, "CMyAudioOutput::OnStopAudioOutput()---> Set m_enState = QAudio::StoppedState[%d]; \n", QAudio::StoppedState);
    m_enState = QAudio::StoppedState;

    //上报状态
//     if (m_enState == QAudio::StoppedState)
//     {
//         LOG(Info, "CMyAudioOutput::OnStartAudioOutput()---> emit signal_updatePlayState( enClose = %d ); \n", enClose);
//         emit signal_updatePlayState(enClose);
//     }

    return;
}

int CMyAudioOutput::state()
{
    return m_enState;
}

qint64 CMyAudioOutput::writeData(const char *data, qint64 len)
{
    LOG(Debug, "CMyAudioOutput::writeData(*data, len=%d)... \n", len);

    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 CMyAudioOutput::readData(char *data, qint64 maxlen)
{
    LOG(Debug, "CMyAudioOutput::readData(char *data, maxlen=%d)... \n", maxlen);

    char *pData = data;
    qint64 iData = maxlen;

    int64_t iDuration = 0;
    int64_t iPTS = 0;
    int64_t iDTS = 0;

    while(iData > 0)
    {
        if(m_pData && m_iData > 0)
        {
            if(m_iData <= iData)
            {
                //LOG(Debug, "CMyAudioOutput::readData()---> memcpy(pData[%d], m_pData[%d]); iData=%d; \n", iData, m_iData, (iData - m_iData));
                memcpy(pData, m_pData, m_iData);
                pData += m_iData;
                iData -= m_iData;

                m_pData = NULL;
                m_iData = 0;
            }
            else
            {
                //LOG(Debug, "CMyAudioOutput::readData()---> memcpy(pData[%d], m_pData[%d]); m_iData=%d; \n", iData, m_iData, (m_iData - iData));
                memcpy(pData, m_pData, iData);
                m_pData += iData;
                m_iData -= iData;

                pData = NULL;
                iData = 0;
            }
        }
        else  //从工作列表取一帧音频
        {
            m_pFrame = getWork();
            if(m_pFrame)
            {
                m_pData = m_pFrame->data;
                m_iData = m_pFrame->iData;
                //LOG(Debug, "CMyAudioOutput::readData()---> getWork()=%p; m_pData=%p; m_iData=%d; \n", m_pFrame, m_pData, m_iData);

                if (iDuration == 0)
                {
                    iPTS = m_pFrame->pts;
                    iDTS = m_pFrame->dts;
                }

                iDuration += m_pFrame->duration;

            }
            else
            {
                LOG(Debug, "CMyAudioOutput::readData()---> memset(pData, 0, iData=%d); m_iBufferDuration=%d; \n", iData, m_iBufferDuration);
                memset(pData, 0, iData);
                pData = NULL;
                iData = 0;

                //iDuration = m_iDuration;

                if (iDuration == 0)
                {
                    iPTS = 0x7FFFFFFF;
                    iDTS = 0x7FFFFFFF;
                }
            }
        }

        //continue;
    }

    LOG(Debug, "CMyAudioOutput::readData()---> iPTS=%d, iDTS=%d, iDuration=%d; \n", iPTS, iDTS, iDuration);

    m_iDuration = iDuration;
    m_iPTS = iPTS;
    m_iDTS = iDTS;

    return maxlen;
}

qint64 CMyAudioOutput::bytesAvailable() const
{
    qint64 iSize = m_iFrameData + QIODevice::bytesAvailable();

    LOG(Debug, "CMyAudioOutput::bytesAvailable() = %l; \n", iSize);

    return iSize;
}

int64_t CMyAudioOutput::getPTS()
{
    return m_iPTS;
}

int64_t CMyAudioOutput::getDTS()
{
    return m_iDTS;
}

int64_t CMyAudioOutput::getDuration()
{
    return m_iDuration;
}

int64_t CMyAudioOutput::getBufferDuration()
{
    return m_iBufferDuration;
}
