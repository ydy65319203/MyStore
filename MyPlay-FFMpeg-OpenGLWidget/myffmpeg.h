#ifndef CMYFFMPEG_H
#define CMYFFMPEG_H

#include "myopenglwidget.h"
#include "myaudio.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <list>

using namespace std;

extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}


class CMyFFmpeg
{
public:
	CMyFFmpeg();
    ~CMyFFmpeg();

    void setVideoOutput(MyOpenGLWidget *pMyVideoOutput);
    void setAudioOutput(CMyAudioOutput *pMyAudioOutput);

    void openAVFile(string &sstrAVFilePath);
    void closeAVFile();
    void Play();
    void Pause();
    //void SeekFrame();  //查找帧

private:
    void thread_OpenAVFile();
    void thread_UnPacket();
    void thread_Video();
    void thread_Audio();

private:
    MyOpenGLWidget *m_pMyVideoOutput;  //视频输出
    CMyAudioOutput *m_pMyAudioOutput;  //音频输出

    string m_sstrAVFilePath;  //AV文件名

    AVFormatContext	*m_pAVFormatCtx;
    AVCodecContext	*m_pVideoCodecCtx;
    AVCodecContext	*m_pAudioCodecCtx;
    AVCodecContext	*m_pSubTitleCodecCtx;

    struct SwsContext *m_pImageConvertCtx;
    struct SwrContext *m_pAudioConvertCtx;

    //AVPacket *m_pAVPacket;
    //AVFrame  *m_pAVFrameYUV;
    //AVFrame  *m_pAVFrame;

    //------------------------------------

    std::list<AVPacket*> m_listPacketBuffer;
    std::list<AVPacket*> m_listPacketVideo;
    std::list<AVPacket*> m_listPacketAudio;

    std::mutex m_MutexPacketBuffer;
    std::mutex m_MutexPacketVideo;
    std::mutex m_MutexPacketAudio;

    size_t m_iPacketMaxCount;  //音视频包最大缓存数量

    double m_dVideoTimebase;  //视频时间基
    double m_dAudioTimebase;

    double m_dVideoDuration;  //视频帧间隔
    double m_dAudioDuration;  //音频帧时长

    int64_t m_iVideoPts;  //当前视频PTS
    int64_t m_iAudioPts;  //当前音频PTS

    int64_t m_iVideoDts;  //当前视频DTS
    int64_t m_iAudioDts;  //当前音频DTS

    int64_t m_iVideoDuration;  //视频包持续时长(*需要乘以时间基)
    int64_t m_iAudioDuration;  //音频包持续时长(*需要乘以时间基)

    int64_t m_iVideoPktDuration;
    int64_t m_iAudioPktDuration;

    uint32_t m_dwVideoDelay;  //视频线程睡眠时长(毫秒)
    uint32_t m_dwAudioDelay;  //音频线程睡眠时长(毫秒)

    int64_t m_iVideoStreamDuration;  //视频流总时长
    int64_t m_iAudioStreamDuration;  //音频流总时长

    int m_iVideoStream;
    int m_iAudioStream;
    int m_iSubTitleStream;

    bool m_bPause;
    bool m_bClose;

    //bool m_bReadPack;
    //bool m_bClickOpenSucc;

    std::thread  m_threadOpenAVFile;
    std::thread  m_threadUnPacket;
    std::thread  m_threadVideo;
    std::thread  m_threadAudio;
};

#endif // CMYFFMPEG_H
