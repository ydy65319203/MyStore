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

    //------------------------------------

    std::list<AVPacket*> m_listPacketBuffer;
    std::list<AVPacket*> m_listPacketVideo;
    std::list<AVPacket*> m_listPacketAudio;

    std::mutex m_MutexPacketBuffer;
    std::mutex m_MutexPacketVideo;
    std::mutex m_MutexPacketAudio;

    int64_t m_iVideoListDuration;  //视频链表总间隔
    int64_t m_iAudioListDuration;  //音频链表总间隔

    double m_dVideoTimebase;  //视频时间基
    double m_dAudioTimebase;

    //------------------------------------------------

    int m_iVideoStream;
    int m_iAudioStream;
    int m_iSubTitleStream;

    bool m_bPause;
    bool m_bClose;

    std::thread  m_threadOpenAVFile;
    std::thread  m_threadUnPacket;
    std::thread  m_threadVideo;
    std::thread  m_threadAudio;
};

#endif // CMYFFMPEG_H
