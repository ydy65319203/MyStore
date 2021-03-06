﻿#ifndef CMYAUDIO_H
#define CMYAUDIO_H

#include <QAudioOutput>

#include <mutex>

class CMyFrame
{
public:
    int64_t pts;
    int64_t dts;
    int64_t duration;
    int64_t iData;
    uint8_t data[0];
} ;

class CMyAudioBuffer
{
public:
    CMyAudioBuffer();
    ~CMyAudioBuffer();

    int setAudioBuffer(int iFrameData, int iFrameCount);
    void clearAudioBuffer();

    void pushFree(CMyFrame *pFrame);
    void pushWork(CMyFrame *pFrame);

    CMyFrame *popFree();
    CMyFrame *popWork();
    CMyFrame *getWork();

//     int64_t getPTS();
//     int64_t getDTS();
//     int64_t getDuration();

    int getFrameData();

protected:
    std::list<CMyFrame*> m_listFree;
    std::list<CMyFrame*> m_listWork;
    std::mutex m_MutexFree;
    std::mutex m_MutexWork;

    CMyFrame *m_pWorkFrame;
    int64_t m_iBufferDuration;
//     int64_t m_iPTS;
//     int64_t m_iDTS;

    uint8_t *m_pFrameBuffer;
    int m_iFrameBuffer;
    int m_iFrameCount;
    int m_iFrameSize;
    int m_iFrameData;
};


class CMyAudioOutput : public QIODevice, public CMyAudioBuffer
{
    Q_OBJECT

public:
    CMyAudioOutput();
    ~CMyAudioOutput();

    void startAudioOutput();
    void pauseAudioOutput();
    void stopAudioOutput();

    void setVolume(int iValue);
    void setReportFlag(bool bReport);  //设置上报标志
    void updatePlayState(int iState, std::string & sstrMessage);  //更新播放状态

    int setAudioFormat(int iChannel, int iSampleRate, int iSampleFormat, int64_t iAudioStreamDuration);

    qint64 writeData(const char *data, qint64 len) override;
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 bytesAvailable() const override;

    int64_t getPTS();
    int64_t getDTS();
    int64_t getDuration();
    int64_t getBufferDuration();  //缓存区内可用音频帧的Duration之和

public slots:
    void OnStartAudioOutput();
    void OnPauseAudioOutput();
    void OnStopAudioOutput();

signals:
    void signal_startAudioOutput();  //用于内部同步
    void signal_pauseAudioOutput();
    void signal_stopAudioOutput();

    void signal_volume(int iVolume);  //向上层应用报告音量
    void signal_updatePlayStep(int iStep, int iReportTotal);  //向上层报告播放进度
    void signal_updatePlayState(int iState, const char *pszMessage);  //向上层应用报告状态

private:
    QAudioFormat  m_audioFormat;    //音频格式
    QAudioOutput *m_pAudioOutput;   //播放设备

    CMyFrame *m_pFrame;
    uint8_t  *m_pData;
    int m_iData;

    int64_t m_iDuration;
    int64_t m_iPts;
    int64_t m_iDts;

    int64_t m_iReportDuration;
    int m_iReportInterval;  //根据时间基计算上报间隔
    int m_iReportTotal;
    bool m_bReportStep;

    bool m_bSetAudioFormat;

    int m_iVolume;

    QAudio::State m_enState;
};

#endif // CMYAUDIO_H
