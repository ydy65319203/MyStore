#ifndef CMYAUDIO_H
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

    void updatePlayState(int iState);

    int setAudioFormat(int iChannel, int iSampleRate, int iSampleFormat);
    //int setFrameBuffer(CMyFrameBuffer *pMyFrameBuffer);
    //int setFrameBuffer(int iFrameData, int iFrameCount);
    int setVolume(int iValue);
    int state();

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
    void signal_startAudioOutput();
    void signal_pauseAudioOutput();
    void signal_stopAudioOutput();

    void signal_updatePlayState(int iState);  //向上层应用报告状态
    void signal_updatePlayStep(int64_t iPts, int64_t iDuratio);  //向上层报告播放进度

private:
    QAudioFormat  m_audioFormat;    //音频格式
    QAudioOutput *m_pAudioOutput;   //播放设备

    CMyFrame *m_pFrame;
    uint8_t  *m_pData;
    int m_iData;

    int64_t m_iDuration;
    int64_t m_iPTS;
    int64_t m_iDTS;

    int m_iVolume;

    QAudio::State m_enState;

    bool m_bSetAudioFormat;
};

#endif // CMYAUDIO_H
