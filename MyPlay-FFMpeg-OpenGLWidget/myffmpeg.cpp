#include "myffmpeg.h"
#include "mylog.h"

//动态库
#pragma comment( lib, "avformat.lib")
#pragma comment( lib, "avdevice.lib")
#pragma comment( lib, "avcodec.lib")
#pragma comment( lib, "swscale.lib")
#pragma comment( lib, "avutil.lib")
#pragma comment( lib, "postproc.lib")
#pragma comment( lib, "swresample.lib")


CMyFFmpeg::CMyFFmpeg()
{
    LOG(Info, "CMyFFmpeg::CMyFFmpeg()---> avdevice_register_all(); \n");
    avdevice_register_all();

    m_pAVFormatCtx = NULL;
    m_pVideoCodecCtx = NULL;
    m_pAudioCodecCtx = NULL;
    m_pSubTitleCodecCtx = NULL;

    m_pImageConvertCtx = NULL;
    m_pAudioConvertCtx = NULL;

    m_pMyVideoOutput = NULL;
    m_pMyAudioOutput = NULL;

    //------------------------------------

    m_iPacketMaxCount = 0;  //音视频包最大缓存数量

    m_dVideoTimebase = 0;
    m_dAudioTimebase = 0;

    m_dVideoDuration = 0;  //视频帧间隔
    m_dAudioDuration = 0;  //音频帧时长

    m_iVideoPts = -9999;
    m_iAudioPts = -9999;

    m_iVideoDuration = 0;
    m_iAudioDuration = 0;

    m_iVideoPktDuration = 0;
    m_iAudioPktDuration = 0;

    m_dwVideoDelay = 0;  //视频线程睡眠时长(毫秒)
    m_dwAudioDelay = 0;  //音频线程睡眠时长(毫秒)

    m_iVideoStream = -1;
    m_iAudioStream = -1;
    m_iSubTitleStream = -1;

    m_bPause = false;
    m_bClose = true;
}

CMyFFmpeg::~CMyFFmpeg()
{

}

void CMyFFmpeg::setVideoOutput(MyOpenGLWidget *pMyVideoOutput)
{
    LOG(Info, "CMyFFmpeg::setVideoOutput( pMyVideoOutput = 0x%p )... \n", pMyVideoOutput);
    m_pMyVideoOutput = pMyVideoOutput;
}

void CMyFFmpeg::setAudioOutput(CMyAudioOutput *pMyAudioOutput)
{
    LOG(Info, "CMyFFmpeg::setAudioOutput( pMyAudioOutput = 0x%p )... \n", pMyAudioOutput);
    m_pMyAudioOutput = pMyAudioOutput;
}

void CMyFFmpeg::openAVFile(string &sstrAVFilePath)
{
    LOG(Info, "CMyFFmpeg::openAVFile()... \n");

    if(m_pMyVideoOutput == NULL || m_pMyAudioOutput == NULL)
    {
        LOG(Error, "CMyFFmpeg::openAVFile()---> m_pMyVideoOutput or m_pMyAudioOutput is NULL \n");
        return;
    }

    m_sstrAVFilePath = sstrAVFilePath;
    m_bClose = true;
    m_bPause = false;

    LOG(Info, "CMyFFmpeg::openAVFile()---> std::thread(&CMyFFmpeg::thread_OpenAVFile, this); \n");
    m_threadOpenAVFile = std::thread(&CMyFFmpeg::thread_OpenAVFile, this);
    m_threadOpenAVFile.detach();
    return;
}

void CMyFFmpeg::Play()
{
    LOG(Info, "CMyFFmpeg::Play()... \n");

    m_bClose = false;
    m_bPause = false;

//    //线程退出
//    if (m_threadOpenAVFile.joinable())
//    {
//        LOG(Info, "CMyFFmpeg::Play()---> m_threadOpenAVFile.join(); \n");
//        m_threadOpenAVFile.join();
//    }

    LOG(Info, "CMyFFmpeg::Play()---> m_iVideoStream=%d, m_iAudioStream=%d; \n", m_iVideoStream, m_iAudioStream);

    //解包线程
    if (0 <= m_iVideoStream || 0 <= m_iAudioStream)
    {
        LOG(Info, "CMyFFmpeg::Play()---> std::thread(&CMyPlay::thread_UnPacket, this); \n");
        m_threadUnPacket = std::thread(&CMyFFmpeg::thread_UnPacket, this);
    }

    //视频线程
    if (0 <= m_iVideoStream)
    {
        LOG(Info, "CMyFFmpeg::Play()---> std::thread(&CMyPlay::thread_Video, this); \n");
        m_threadVideo = std::thread(&CMyFFmpeg::thread_Video, this);
    }

    //音频线程
    if (0 <= m_iAudioStream)
    {
        LOG(Info, "CMyFFmpeg::Play()---> std::thread(&CMyPlay::thread_Audio, this); \n");
        m_threadAudio = std::thread(&CMyFFmpeg::thread_Audio, this);
    }

    LOG(Info, "CMyFFmpeg::Play() End \n");
}

void CMyFFmpeg::Pause()
{
    LOG(Info, "CMyFFmpeg::Pause()... \n");
    m_bPause = !m_bPause;

    if (m_bPause)
    {
        if (m_pMyVideoOutput)
        {
            LOG(Info, "CMyFFmpeg::Pause()---> m_pMyVideoOutput->updatePlayState(enPause=%d); \n", enPause);
            m_pMyVideoOutput->updatePlayState(enPause);
        }
        else if (m_pMyAudioOutput)
        {
            LOG(Info, "CMyFFmpeg::Pause()---> m_pMyAudioOutput->updatePlayState(enPause=%d); \n", enPause);
            m_pMyAudioOutput->updatePlayState(enPause);
        }

        if (m_pMyAudioOutput)
        {
            LOG(Info, "CMyFFmpeg::Pause()---> m_pMyAudioOutput->pauseAudioOutput(); \n");
            m_pMyAudioOutput->pauseAudioOutput();
        }
    }
    else
    {
        if (m_pMyVideoOutput)
        {
            LOG(Info, "CMyFFmpeg::Pause()---> m_pMyVideoOutput->updatePlayState(enPlay=%d); \n", enPlay);
            m_pMyVideoOutput->updatePlayState(enPlay);
        }
        else if (m_pMyAudioOutput)
        {
            LOG(Info, "CMyFFmpeg::Pause()---> m_pMyAudioOutput->updatePlayState(enPlay=%d); \n", enPlay);
            m_pMyAudioOutput->updatePlayState(enPlay);
        }

        if (m_pMyAudioOutput)
        {
            LOG(Info, "CMyFFmpeg::Pause()---> m_pMyAudioOutput->pauseAudioOutput(); \n");
            m_pMyAudioOutput->pauseAudioOutput();
        }
    }
}

void CMyFFmpeg::closeAVFile()
{
    LOG(Info, "CMyFFmpeg::closeAVFile()... \r\n");

    m_bClose = true;
    m_bPause = false;

    Sleep(300);

    //线程退出
    if (m_threadUnPacket.joinable())
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> m_threadUnPacket.join(); \r\n");
        m_threadUnPacket.join();
    }

    if (m_threadVideo.joinable())
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> m_threadVideo.join(); \r\n");
        m_threadVideo.join();
    }

    if (m_threadAudio.joinable())
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> m_threadAudio.join(); \r\n");
        m_threadAudio.join();
    }

    //-------------------------------------------------------------

    AVPacket *pAVPacket = NULL;
    std::list<AVPacket*>::iterator iter;

    //视频
    m_MutexPacketVideo.lock();  //--加锁
    LOG(Info, "CMyFFmpeg::closeAVFile()---> listPacketVideo.size()=%d; \r\n", m_listPacketVideo.size());
    for (iter = m_listPacketVideo.begin(); iter != m_listPacketVideo.end(); ++iter)
    {
        pAVPacket = *iter;
        av_packet_free(&pAVPacket);  //释放AVPacket
    }
    m_listPacketVideo.clear();
    m_MutexPacketVideo.unlock();  //--解锁

    //音频
    m_MutexPacketAudio.lock();  //--加锁
    LOG(Info, "CMyFFmpeg::closeAVFile()---> listPacketAudio.size()=%d; \r\n", m_listPacketAudio.size());
    for (iter = m_listPacketAudio.begin(); iter != m_listPacketAudio.end(); ++iter)
    {
        pAVPacket = *iter;
        av_packet_free(&pAVPacket);  //释放AVPacket
    }
    m_listPacketAudio.clear();
    m_MutexPacketAudio.unlock();  //--解锁

    //缓存
    m_MutexPacketBuffer.lock();  //--加锁
    LOG(Info, "CMyFFmpeg::closeAVFile()---> listPacketBuffer.size()=%d; \r\n", m_listPacketBuffer.size());
    for (iter = m_listPacketBuffer.begin(); iter != m_listPacketBuffer.end(); ++iter)
    {
        pAVPacket = *iter;
        av_packet_free(&pAVPacket);  //释放AVPacket
    }
    m_listPacketBuffer.clear();
    m_MutexPacketBuffer.unlock();  //--解锁

    //-----------------------------------------------------

    //释放转换器
    if (m_pImageConvertCtx)
    {
        sws_freeContext(m_pImageConvertCtx);
        m_pImageConvertCtx = NULL;
    }

    if (m_pAudioConvertCtx)
    {
        swr_free(&m_pAudioConvertCtx);
        m_pAudioConvertCtx = NULL;
    }

    //释放解码器上下文环境
    if (m_pVideoCodecCtx)
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> avcodec_free_context(&m_pVideoCodecCtx); \r\n");
        avcodec_free_context(&m_pVideoCodecCtx);
    }

    if (m_pAudioCodecCtx)
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> avcodec_free_context(&m_pAudioCodecCtx); \r\n");
        avcodec_free_context(&m_pAudioCodecCtx);
    }

    if (m_pSubTitleCodecCtx)
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> avcodec_free_context(&m_pSubTitleCodecCtx); \r\n");
        avcodec_free_context(&m_pSubTitleCodecCtx);
    }

    //释放AV容器
    if (m_pAVFormatCtx)
    {
        LOG(Info, "CMyFFmpeg::closeAVFile()---> avformat_free_context(m_pAVFormatCtx); \r\n");
        //avformat_flush(m_pAVFormatCtx);
        //avformat_close_input(&m_pAVFormatCtx);
        avformat_free_context(m_pAVFormatCtx);
        m_pAVFormatCtx = NULL;
    }

    //---------------------------------------------------

    m_iPacketMaxCount = 0;  //音视频包最大缓存数量

    m_dVideoTimebase = 0;
    m_dAudioTimebase = 0;

    m_dVideoDuration = 0;  //视频帧间隔
    m_dAudioDuration = 0;  //音频帧时长

    m_iVideoPts = -9999;
    m_iAudioPts = -9999;

    m_iVideoDuration = 0;
    m_iAudioDuration = 0;

    m_iVideoPktDuration = 0;
    m_iAudioPktDuration = 0;

    m_dwVideoDelay = 0;  //视频线程睡眠时长(毫秒)
    m_dwAudioDelay = 0;  //音频线程睡眠时长(毫秒)

    m_iVideoStream = -1;
    m_iAudioStream = -1;
    m_iSubTitleStream = -1;
}

void CMyFFmpeg::thread_OpenAVFile()
{
    LOG(Info, "CMyFFmpeg::thread_OpenAVFile()... \n");

    //清理环境
    this->closeAVFile();

    //重新分配上下文空间
    m_pAVFormatCtx = avformat_alloc_context();
    if (m_pAVFormatCtx == NULL)
    {
        LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avformat_alloc_context() = NULL; \n");
        return;
    }

    //m_pAVFormatCtx->flags |= AVFMT_FLAG_NONBLOCK;

    //打开设备
    //LOG("avformat_open_input(video=e2eSoft VCam); \n");
    //iRet = avformat_open_input(&m_pAVFormatCtx, "video=e2eSoft VCam", pInputFormat, NULL);
    //int iRet = avformat_open_input(&m_pAVFormatCtx, "audio=麦克风 (Realtek High Definition Au", pInputFormat, NULL);  //中文转UTF8

    //打开文件
    //m_sstrPathFile = "D:\\YDY\\AVFile\\Pigger02.mp4";
    LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> avformat_open_input(%s); \n", m_sstrAVFilePath.c_str());
    int iRet = avformat_open_input(&m_pAVFormatCtx, m_sstrAVFilePath.c_str(), NULL, NULL);
    if (iRet < 0)
    {
        LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avformat_open_input() Fail; \n");
        return;
    }

    //探测流信息
    LOG(Debug, "CMyFFmpeg::thread_OpenAVFile()---> avformat_find_stream_info(); \n");
    iRet = avformat_find_stream_info(m_pAVFormatCtx, NULL);
    if (iRet < 0)
    {
        LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avformat_find_stream_info() Fail; \n");
        return;
    }

    //计算总时长
    int64_t iTotalSeconds = m_pAVFormatCtx->duration / AV_TIME_BASE; //S
    int iHour = iTotalSeconds / 3600;//小时
    int iMinute = iTotalSeconds % 3600 / 60;//分钟
    int iSecond = iTotalSeconds % 60;//秒

    LOG(Info, "AVFormatContext: duration=%ld [%02d:%02d:%02d], nb_streams=%d, start_time=%d, bit_rate=%d, packet_size=%d, max_delay=%d, \n",
        m_pAVFormatCtx->duration,
        iHour, iMinute, iSecond,
        m_pAVFormatCtx->nb_streams,
        m_pAVFormatCtx->start_time,
        m_pAVFormatCtx->bit_rate,
        m_pAVFormatCtx->packet_size,
        m_pAVFormatCtx->max_delay);

    //m_pAVFormatCtx->pb->error;

    //av_dump_format(m_pAVFormatCtx, 0, m_sstrPathFile.c_str(), false);
    //-----------------------------------------------------------------

    AVCodecParameters *pCodecPar = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVStream *pStream = NULL;
    AVCodec *pCodec = NULL;

    //分解流
    for (unsigned int iIndex = 0; iIndex < m_pAVFormatCtx->nb_streams; ++iIndex)
    {
        pStream = m_pAVFormatCtx->streams[iIndex];

        LOG(Info, "AVStream[%d]: time_base=%d/%d, start_time=%lld, duration=%lld, duration=%p; nb_frames=%lld, sample_aspect_ratio=%d/%d, avg_frame_rate=%d/%d, r_frame_rate=%d/%d; \n",
            pStream->index,
            pStream->time_base.num,
            pStream->time_base.den,
            pStream->start_time,
            pStream->duration,
            pStream->duration,

            pStream->nb_frames,
            pStream->sample_aspect_ratio.num,
            pStream->sample_aspect_ratio.den,
            pStream->avg_frame_rate.num,
            pStream->avg_frame_rate.den,
            pStream->r_frame_rate.num,
            pStream->r_frame_rate.den);

        //解码器
        pCodecPar = pStream->codecpar;

        //查找解码器
        pCodec = avcodec_find_decoder(pCodecPar->codec_id);
        if (pCodec == NULL)
        {
            LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_find_decoder(codec_id=%d) Fail; \n", pCodecPar->codec_id);
            continue;
        }

        //解码器上下文
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (pCodecCtx == NULL)
        {
            LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_alloc_context3() Fail; \n");
            continue;
        }

        //拷贝参数
        iRet = avcodec_parameters_to_context(pCodecCtx, pCodecPar);
        if (iRet < 0)
        {
            LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_parameters_to_context() Fail; \n");
            continue;
        }

        //设置解码线程数量
        //pCodecCtx->thread_count = 2;

        //打开上下文
        iRet = avcodec_open2(pCodecCtx, pCodec, NULL);
        if (iRet < 0)
        {
            LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_open2() Fail; \n");
            continue;
        }

        LOG(Info, "AVCodecContext: time_base=%d/%d, ticks_per_frame=%d, name=%s, codec_id=%d, codec_type=%d, bit_rate=%lld, delay=%d, width=%d, height=%d, coded_width=%d, coded_height=%d, gop_size=%d, pix_fmt=%d, sample_fmt=%d, sample_rate=%d, channels=%d, channel_layout=%d, frame_size=%d, frame_number=%d, framerate=%d/%d; \n",
            pCodecCtx->time_base.num,
            pCodecCtx->time_base.den,
            pCodecCtx->ticks_per_frame,

            pCodec->name,
            pCodecCtx->codec_id,
            pCodecCtx->codec_type,
            pCodecCtx->bit_rate,

            pCodecCtx->delay,
            pCodecCtx->width,
            pCodecCtx->height,
            pCodecCtx->coded_width,
            pCodecCtx->coded_height,

            pCodecCtx->gop_size,
            pCodecCtx->pix_fmt,
            pCodecCtx->sample_fmt,
            pCodecCtx->sample_rate,

            pCodecCtx->channels,
            pCodecCtx->channel_layout,
            pCodecCtx->frame_size,
            pCodecCtx->frame_number,
            pCodecCtx->framerate.num,
            pCodecCtx->framerate.den);

        //--------------------------------------------------------

        if (pCodecPar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> pCodecPar->codec_type = AVMEDIA_TYPE_VIDEO[%d]; \n", AVMEDIA_TYPE_VIDEO);

            if(m_pMyVideoOutput)
            {
                //保存流索引
                m_iVideoStream = iIndex;
                m_pVideoCodecCtx = pCodecCtx;

                //视频流时长
                m_iVideoStreamDuration = pStream->duration;
                if(m_iVideoStreamDuration < 1)
                {
                    //取容器时长，按照流的时间基转换。
                    //AVRational AVTimeBaseQ = AV_TIME_BASE_Q;
                    AVRational AVTimeBase;
                    AVTimeBase.num = 1;
                    AVTimeBase.den = AV_TIME_BASE;
                    m_iVideoStreamDuration = av_rescale_q(m_pAVFormatCtx->duration, AVTimeBase, pStream->time_base);
                }

                //视频帧时间基
                m_dVideoTimebase = av_q2d(pStream->time_base) * 1000;
                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_iVideoStreamDuration = %p; m_dVideoTimebase = av_q2d(pAVStream->time_base=%d/%d) * 1000 = %fms; \n",
                          m_iVideoStreamDuration,
                          pStream->time_base.num,
                          pStream->time_base.den,
                          m_dVideoTimebase);

                //上报视频流总时长
                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyVideoOutput->setAudioStreamDuration(num=%d, den=%d, m_iVideoStreamDuration=%p); \n", pStream->time_base.num, pStream->time_base.den, m_iVideoStreamDuration);
                m_pMyVideoOutput->setVideoStreamDuration(pStream->time_base.num, pStream->time_base.den, m_iVideoStreamDuration);

                //Enable播放按钮
                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyVideoOutput->updatePlayState(enOpen=%d); \n", enOpen);
                m_pMyVideoOutput->updatePlayState(enOpen);
            }
            else
            {
                LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyVideoOutput = NULL; Set m_iVideoStream = -2; \n");
                m_iVideoStream = -2;

                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_free_context(&pCodecCtx); \n");
                avcodec_free_context(&pCodecCtx);
            }
        }
        else if (pCodecPar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> pCodecPar->codec_type = AVMEDIA_TYPE_AUDIO[%d]; \n", AVMEDIA_TYPE_AUDIO);

            if(m_pMyAudioOutput)
            {
                //保存流索引
                m_iAudioStream = iIndex;
                m_pAudioCodecCtx = pCodecCtx;

                //视频流时长
                m_iAudioStreamDuration = pStream->duration;
                if(m_iAudioStreamDuration < 1)
                {
                    //取容器时长，按照流的时间基转换。
                    AVRational AVTimeBase;
                    AVTimeBase.num = 1;
                    AVTimeBase.den = AV_TIME_BASE;
                    m_iAudioStreamDuration = av_rescale_q(m_pAVFormatCtx->duration, AVTimeBase, pStream->time_base);
                }

                //音频帧时间基
                m_dAudioTimebase = av_q2d(pStream->time_base) * 1000;
                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_iAudioStreamDuration = %p; m_dAudioTimebase = av_q2d(pAVStream->time_base=%d/%d) * 1000 = %fms; \n",
                          m_iAudioStreamDuration,
                          pStream->time_base.num,
                          pStream->time_base.den,
                          m_dAudioTimebase);

                //上报音频流总时长
                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyAudioOutput->setAudioStreamDuration(num=%d, den=%d, m_iAudioStreamDuration=%p); \n", pStream->time_base.num, pStream->time_base.den, m_iAudioStreamDuration);
                m_pMyAudioOutput->setAudioStreamDuration(pStream->time_base.num, pStream->time_base.den, m_iAudioStreamDuration);

                //上报播放状态
                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyAudioOutput->updatePlayState(enOpen=%d); \n", enOpen);
                m_pMyAudioOutput->updatePlayState(enOpen);
            }
            else
            {
                LOG(Error, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyAudioOutput = NULL; Set m_iAudioStream = -2; \n");
                m_iAudioStream = -2;

                LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_free_context(&pCodecCtx); \n");
                avcodec_free_context(&pCodecCtx);
            }
        }
        else if (pCodecPar->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> pCodecPar->codec_type = AVMEDIA_TYPE_SUBTITLE[%d]; \n", AVMEDIA_TYPE_SUBTITLE);
            m_pSubTitleCodecCtx = pCodecCtx;
            //m_iSubTitleStream = iIndex;
        }
        else
        {
            LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> pCodecPar->codec_type = %d; Unkonw ! \n", pCodecPar->codec_type);
            LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> avcodec_free_context(&pCodecCtx); \n");
            avcodec_free_context(&pCodecCtx);
        }

        LOG(Info, "\n");
    }

    //选择上报设备，音频优先。 //int64_t av_rescale_q(int64_t a, AVRational bq, AVRational cq) av_const;
    if(m_iAudioStream >= 0)
    {
        LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyAudioOutput->setReportFlag(true); \n");
        m_pMyVideoOutput->setReportFlag(false);
        m_pMyAudioOutput->setReportFlag(true);
    }
    else if(m_iVideoStream >= 0)
    {
        LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> m_pMyVideoOutput->setReportFlag(true); \n");
        m_pMyVideoOutput->setReportFlag(true);
        m_pMyAudioOutput->setReportFlag(false);
    }
    else
    {
        LOG(Info, "CMyFFmpeg::thread_OpenAVFile()---> (m_pMyVideoOutput and m_pMyAudioOutput)->setReportFlag(false); \n");
        m_pMyVideoOutput->setReportFlag(false);
        m_pMyAudioOutput->setReportFlag(false);
    }

    //-----------------------------------------------------------------
    //LOG(Info, "CMyFFmpeg::thread_OpenAVFile() End \n");
}


void CMyFFmpeg::thread_UnPacket()
{
    LOG(Info, "CMyFFmpeg::thread_UnPacket()... \n");

    if (m_pAVFormatCtx == NULL)
    {
        LOG(Error, "CMyFFmpeg::thread_UnPacket()---> m_pAVFormatCtx = NULL; return; \n");
        return;
    }

    AVPacket *pAVPacket = NULL;
    size_t iPacketVideo = 0;
    size_t iPacketAudio = 0;

    int iVideoFullDelay = 0;  //视频队列满载时睡眠时长(毫秒)
    int iAudioFullDelay = 0;
    int iDelay = 0;

    int iPacketState = 0;

    m_iPacketMaxCount = 25;  //音视频包最大缓存数量
 

    while (!m_bClose)
    {
        //暂停播放
        if (m_bPause)
        {
            //LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> m_bPause = true; Sleep(200); \n");
            Sleep(200);
            continue;
        }

        //视频队列满
        if (iPacketVideo >= m_iPacketMaxCount)
        {
            //计算视频队列满时长
            if (iVideoFullDelay < 100)
            {
                iVideoFullDelay = (int)(m_dVideoTimebase * m_iVideoPktDuration * (iPacketVideo - 2));
                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> (m_dVideoTimebase[%lf] * m_iVideoPktDuration[%d] * iPacketVideo[%d - 2]) = iVideoFullDelay[%d]; \n", m_dVideoTimebase, m_iVideoPktDuration, iPacketVideo, iVideoFullDelay);
            }

            //计算音频队列时长
            int iAudioDelay = (int)(m_dAudioTimebase * m_iAudioPktDuration * iPacketAudio * 0.5);
            if (0 < iAudioDelay && iAudioDelay < iVideoFullDelay)
            {
                iDelay = iAudioDelay;
            }
            else
            {
                iDelay = iVideoFullDelay;
            }

            //睡一会儿
            if (iDelay > 40)
            {
                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> iVideoDelay[%d]=%d; iAudioDelay[%d]=%d; Sleep(%dms); \n\n", iPacketVideo, iVideoFullDelay, iPacketAudio, iAudioDelay, iDelay);
                Sleep(iDelay);
            }
            else
            {
                iDelay = 40;
                LOG(Warn, "CMyFFmpeg::thread_UnPacket()---> iVideoDelay[%d]=%d; iAudioDelay[%d]=%d; Sleep(%dms); \n\n", iPacketVideo, iVideoFullDelay, iPacketAudio, iAudioDelay, iDelay);
                Sleep(iDelay);
            }

            //视频Packet列表
            m_MutexPacketVideo.lock();  //--加锁
            iPacketVideo = m_listPacketVideo.size();
            m_MutexPacketVideo.unlock();  //--解锁

            //继续
            continue;
        }

        //音频队列满
        if(iPacketAudio >= m_iPacketMaxCount)
        {
            //计算音频队列满时长
            if (iAudioFullDelay < 100)
            {
                iAudioFullDelay = (int)(m_dAudioTimebase * m_iAudioPktDuration * iPacketAudio * 0.5);
                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> (m_dAudioTimebase[%lf] * m_iAudioPktDuration[%d] * iPacketAudio[%d] * 0.5) = iAudioFullDelay[%d]; \n", m_dAudioTimebase, m_iAudioPktDuration, iPacketAudio, iAudioFullDelay);
            }

            //计算视频队列时长
            int iVideoDelay = (int)(m_dVideoTimebase * m_iVideoPktDuration * (iPacketVideo - 2));
            if (0 < iVideoDelay && iVideoDelay < iAudioFullDelay)
            {
                iDelay = iVideoDelay;
            }
            else
            {
                iDelay = iAudioFullDelay;
            }

            //睡一会儿
            if (iDelay > 10)
            {
                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> iVideoDelay[%d]=%d; iAudioDelay[%d]=%d; Sleep(%dms); \n\n", iPacketVideo, iVideoDelay, iPacketAudio, iAudioFullDelay, iDelay);
                Sleep(iDelay);
            }
            else
            {
                iDelay = 50;
                LOG(Warn, "CMyFFmpeg::thread_UnPacket()---> iVideoDelay[%d]=%d; iAudioDelay[%d]=%d; Sleep(%dms); \n\n", iPacketVideo, iVideoDelay, iPacketAudio, iAudioFullDelay, iDelay);
                Sleep(iDelay);
            }

            //音频Packet列表
            m_MutexPacketAudio.lock();  //--加锁
            iPacketAudio = m_listPacketAudio.size();
            m_MutexPacketAudio.unlock();  //--解锁

            //继续
            continue;
        }

        //---------------------------------------------------------------------

        if(pAVPacket == NULL)
        {
            //空闲Packet列表
            m_MutexPacketBuffer.lock();  //--加锁
            if (m_listPacketBuffer.size() > 0)
            {
                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> pAVPacket = listPacketBuffer.pop_front(); \n");
                pAVPacket = m_listPacketBuffer.front();
                m_listPacketBuffer.pop_front();
            }
            m_MutexPacketBuffer.unlock();  //--解锁

            //申请AVPacket
            if(pAVPacket == NULL)
            {
                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> m_listPacketBuffer.size()=0; pAVPacket = av_packet_alloc(); \n");
                pAVPacket = av_packet_alloc();
                if (pAVPacket == NULL)
                {
                    LOG(Error, "CMyFFmpeg::thread_UnPacket()---> av_packet_alloc() = NULL; Set m_bClose = true; \n");
                    m_bClose = true;
                    break;
                }
            }
        }

        //---------------------------------------------------------------------

        //读取AVPacket
        iPacketState = av_read_frame(m_pAVFormatCtx, pAVPacket);
        if (0 == iPacketState)
        {
            LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> av_read_frame(m_pAVFormatCtx, pAVPacket) = %d; \n", iPacketState);

            //写日志
            LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> AVPacket[%d]: time_base=%d/%d, flags=0x%x, pts=%ld, dts=%ld, duration=%ld, pos=%ld, size=%d; \r\n",
                    pAVPacket->stream_index,
                    m_pAVFormatCtx->streams[pAVPacket->stream_index]->time_base.num,
                    m_pAVFormatCtx->streams[pAVPacket->stream_index]->time_base.den,
                    pAVPacket->flags,  //if (pAVPacket->flags & AV_PKT_FLAG_KEY);  //关键帧
                    pAVPacket->pts,
                    pAVPacket->dts,
                    pAVPacket->duration,
                    pAVPacket->pos,
                    pAVPacket->size);

            if (pAVPacket->stream_index == m_iVideoStream)  //视频Packet
            {
                //缓存AVPacket
                m_MutexPacketVideo.lock();  //--加锁
                m_listPacketVideo.push_back(pAVPacket);
                iPacketVideo = m_listPacketVideo.size();
                m_MutexPacketVideo.unlock();  //--解锁

                //保存Duration
                m_iVideoPktDuration = pAVPacket->duration;

                pAVPacket = NULL;

                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> m_listPacketVideo.push_back(pAVPacket); m_listPacketVideo.size()=%d; \n", iPacketVideo);

            }
            else if (pAVPacket->stream_index == m_iAudioStream)  //音频Packet
            {
                //缓存AVPacket
                m_MutexPacketAudio.lock();  //--加锁
                m_listPacketAudio.push_back(pAVPacket);
                iPacketAudio = m_listPacketAudio.size();
                m_MutexPacketAudio.unlock();  //--解锁

                //保存Duration
                m_iAudioPktDuration = pAVPacket->duration;

                pAVPacket = NULL;

                LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> m_listPacketAudio.push_back(pAVPacket); m_listPacketAudio.size()=%d; \n", iPacketAudio);
            }
            else  //未定义的类型
            {
                LOG(Warn, "CMyFFmpeg::thread_UnPacket()---> Undefine pAVPacket->stream_index = %d; av_packet_unref(pAVPacket); \n", pAVPacket->stream_index);
                av_packet_unref(pAVPacket);  //解引用AVPacket
            }

            LOG(Debug, "\n");

            continue;
        }
        else if (AVERROR_EOF == iPacketState)  //读到文件尾
        {
            LOG(Debug, "CMyFFmpeg::thread_UnPacket()---> av_read_frame(m_pAVFormatCtx, pAVPacket) = AVERROR_EOF; listPacketVideo.push_back(NULL); listPacketAudio.push_back(NULL); \n");
            if (-1 < m_iVideoStream)
            {
                m_MutexPacketVideo.lock();  //--加锁
                m_listPacketVideo.push_back(NULL);
                m_MutexPacketVideo.unlock();  //--解锁
            }

            if (-1 < m_iAudioStream)
            {
                m_MutexPacketAudio.lock();  //--加锁
                m_listPacketAudio.push_back(NULL);
                m_MutexPacketAudio.unlock();  //--解锁
            }

            break;
        }
        else  //读取出错了
        {
            LOG(Error, "CMyFFmpeg::thread_UnPacket()---> av_read_frame(m_pAVFormatCtx, pAVPacket) = %d; break; \n", iPacketState);
            break;
        }
    }

    //释放AVPacket
    if(pAVPacket)
    {
        av_packet_free(&pAVPacket);
        pAVPacket = NULL;
    }

    LOG(Info, "CMyFFmpeg::thread_UnPacket() End \n");
}


void CMyFFmpeg::thread_Video()
{
    LOG(Info, "CMyFFmpeg::thread_Video()... \n");

    if(m_pMyVideoOutput == NULL)
    {
        LOG(Error, "CMyFFmpeg::thread_Video()---> m_pMyVideoOutput = NULL; \n");
        return;
    }

    //准备AVFrame
    AVFrame *pAVFrame = av_frame_alloc();
    AVFrame *pAVFrameYUV = av_frame_alloc();
    if (m_pVideoCodecCtx == NULL || pAVFrameYUV == NULL || pAVFrame == NULL)
    {
        LOG(Error, "CMyFFmpeg::thread_Video()---> av_frame_alloc() = NULL; \n");
        return;
    }

    int iRet = av_image_alloc(pAVFrameYUV->data, pAVFrameYUV->linesize,
                              m_pVideoCodecCtx->width, m_pVideoCodecCtx->height,
                              AV_PIX_FMT_YUV420P, 1);

    LOG(Info, "CMyFFmpeg::thread_Video()---> pAVFrameYUV: av_image_alloc(AV_PIX_FMT_YUV420P, width=%d, height=%d) = %d; \n", m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, iRet);
    LOG(Info, "CMyFFmpeg::thread_Video()---> pAVFrameYUV->data[0]=%p, data[1]=%p, data[2]=%p, data[3]=%d;  \n", pAVFrameYUV->data[0], pAVFrameYUV->data[1], pAVFrameYUV->data[2], pAVFrameYUV->data[3]);
    LOG(Info, "CMyFFmpeg::thread_Video()---> pAVFrameYUV->linesize[0]=%d, linesize[1]=%d, linesize[2]=%d, linesize[3]=%d;  \n", pAVFrameYUV->linesize[0], pAVFrameYUV->linesize[1], pAVFrameYUV->linesize[2], pAVFrameYUV->linesize[3]);

    std::list<AVPacket*> listPacket;
    AVPacket *pAVPacket = NULL;
    int iReadState = 0;
    int iSendState = 0;

    while (!m_bClose)
    {
        //暂停播放
        if (m_bPause)
        {
            //LOG(Debug, "CMyFFmpeg::thread_Video()---> m_bPause = true; Sleep(200); \n");
            Sleep(200);
            continue;
        }

        //----------------------------------------------

        LOG(Debug, "\n");

        //从解码器读取Frame
        iReadState = avcodec_receive_frame(m_pVideoCodecCtx, pAVFrame);
        if(iReadState == AVERROR_EOF)  // --- 解码器读尽，播放线程退出。---
        {
            LOG(Info, "CMyFFmpeg::thread_Video()---> avcodec_receive_frame() = AVERROR_EOF; m_pMyVideoOutput->updatePlayState(enClose=%d); break; \n", enClose);
            m_pMyVideoOutput->updatePlayState(enClose);
            break;
        }
        else if(iReadState == AVERROR(EINVAL))  //codec not opened, or it is an encoder
        {
            LOG(Info, "CMyFFmpeg::thread_Video()---> avcodec_receive_frame() = AVERROR(EINVAL); break; \n");
            break;
        }
        else if (iReadState == 0)  //读取到数据
        {
            LOG(Debug, "CMyFFmpeg::thread_Video()---> avcodec_receive_frame() = 0; \n");

            LOG(Debug, "CMyFFmpeg::thread_Video()---> AVFrame[Video]: best_effort_timestamp=%lld, pts=%lld, format=%d, key_frame=%d, pict_type=%d, width=%d, height=%d; "
                   "pkt_pos=%lld, pkt_dts=%lld, pkt_duration=%lld, pkt_size=%d; "
                   "coded_picture_number=%d, display_picture_number=%d, repeat_pict=%d, interlaced_frame=%d; top_field_first=%d, flag=%d; \n",
                   pAVFrame->best_effort_timestamp,
                   pAVFrame->pts,
                   pAVFrame->format,

                   pAVFrame->key_frame,
                   pAVFrame->pict_type,
                   pAVFrame->width,
                   pAVFrame->height,

                   pAVFrame->pkt_pos,
                   pAVFrame->pkt_dts,
                   pAVFrame->pkt_duration,
                   pAVFrame->pkt_size,

                   pAVFrame->coded_picture_number,
                   pAVFrame->display_picture_number,
                   pAVFrame->repeat_pict,
                   pAVFrame->interlaced_frame,
                   pAVFrame->top_field_first,
                   pAVFrame->flags);

           if(m_pImageConvertCtx == NULL)
           {
               //格式转换上下文
               LOG(Info, "CMyFFmpeg::thread_Video()---> m_pImageConvertCtx = sws_getContext(width=%d, height=%d, AV_PIX_FMT_YUV420P, SWS_BICUBIC); \n", pAVFrame->width, pAVFrame->height);
               m_pImageConvertCtx = sws_getContext(pAVFrame->width, pAVFrame->height,
                                                   m_pVideoCodecCtx->pix_fmt,
                                                   pAVFrame->width,
                                                   pAVFrame->height,
                                                   AV_PIX_FMT_YUV420P,
                                                   SWS_BICUBIC,   //缩放算法
                                                   NULL, NULL, NULL);
               if (m_pImageConvertCtx == NULL)
               {
                   LOG(Error, "CMyFFmpeg::thread_Video()---> sws_getContext(width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC) = NULL; break; \n");
                   break;
               }

               //设置图像格式和尺寸
               LOG(Info, "CMyFFmpeg::thread_Video()---> m_pMyVideoOutput->setVideoFormat(AV_PIX_FMT_YUV420P=0, width=%d, height=%d); \n", pAVFrame->width, pAVFrame->height);
               m_pMyVideoOutput->setVideoFormat(AV_PIX_FMT_YUV420P, pAVFrame->width, pAVFrame->height);
           }

           //转换像素格式 return the height of the output slice
           iRet = sws_scale(m_pImageConvertCtx, (const unsigned char* const*)pAVFrame->data, pAVFrame->linesize,
                            0, m_pVideoCodecCtx->height,  //定义输入图像上的处理区域，srcSliceY是起始位置，srcSliceH是处理多少行
                            pAVFrameYUV->data, pAVFrameYUV->linesize);
           LOG(Debug, "CMyFFmpeg::thread_Video()---> sws_scale() = %d; \n", iRet);

           //显示图像
           LOG(Debug, "CMyFFmpeg::thread_Video()---> m_pMyVideoOutput->updateVideoData(pAVFrameYUV->data[0]); \n");
           m_pMyVideoOutput->updateVideoData(pAVFrameYUV->data[0], pAVFrame->pts, pAVFrame->pkt_duration); //(pAVFrameYUV->data[0]);

           //------------------------------------------------------------------

           //与音频同步
           do
           {
               //保存旧的视频PTS
               int64_t iVideoPts = m_iVideoPts;

               //取新时间戳
               m_iVideoPts = pAVFrame->pts;
               m_iVideoDts = pAVFrame->pkt_dts;
               if(m_iVideoDuration != pAVFrame->pkt_duration)
               {
                   m_iVideoDuration = pAVFrame->pkt_duration;
                   m_dVideoDuration = m_iVideoDuration * m_dVideoTimebase;
                   m_dwVideoDelay   = (uint32_t)m_dVideoDuration;

                   LOG(Debug, "CMyFFmpeg::thread_Video()---> Update: m_dVideoTimebase[%lf] * m_iVideoDuration[%d] = m_dVideoDuration[%lf]; m_dwVideoDelay=%d; \n", m_dVideoTimebase, m_iVideoDuration, m_dVideoDuration, m_dwVideoDelay);
               }

               //视频pts异常，按常速播放。
               if (iVideoPts >= pAVFrame->pts)
               {
                   LOG(Warn, "CMyFFmpeg::thread_Video()---> Video pts abnormal: m_iVideoPts[%d] >= pAVFrame->pts[%d]; \n", iVideoPts, pAVFrame->pts);
                   break;
               }

               //---------------------------------------------------------------

               if(m_iAudioStream < 0)
               {
                   LOG(Debug, "CMyFFmpeg::thread_Video()---> m_iAudioStream = %d; Not audio \n", m_iAudioStream);
                   break;
               }
               else if(m_pMyAudioOutput == NULL) // || m_pMyAudioOutput->state() != QAudio::ActiveState)
               {
                   LOG(Debug, "CMyFFmpeg::thread_Video()---> m_pMyAudioOutput not ready! \n");
                   break;
               }

               //保存旧的音频PTS
               int64_t iAudioPts = m_iAudioPts;

               //取音频时间戳
               m_iAudioPts = m_pMyAudioOutput->getPTS();
               m_iAudioDts = m_pMyAudioOutput->getDTS();
               if(m_iAudioDuration != m_pMyAudioOutput->getDuration())
               {
                   m_iAudioDuration = m_pMyAudioOutput->getDuration();
                   m_dAudioDuration = m_iAudioDuration * m_dAudioTimebase;
                   m_dwAudioDelay   = (uint32_t)m_dAudioDuration;

                   LOG(Debug, "CMyFFmpeg::thread_Video()---> Update: m_iAudioDuration=%d; m_dAudioDuration=%lf; m_dwAudioDelay=%d; \n", m_iAudioDuration, m_dAudioDuration, m_dwAudioDelay);
                   //LOG(Debug, "CMyFFmpeg::thread_Video()---> m_iVideoDuration[%d] * m_dVideoTimebase[%lf] = m_dVideoDuration[%lf]; m_dwVideoDelay=%d; \n", m_iVideoDuration, m_dVideoTimebase, m_dVideoDuration, m_dwVideoDelay);
               }

               if (iAudioPts > m_iAudioPts || m_iAudioPts > 0x7FFFFF00)
               {
                   //音频pts异常，按常速播放。
                   LOG(Warn, "CMyFFmpeg::thread_Video()---> Audio pts abnormal: m_iAudioPts[%d] > pAVFrame->pts[%d] \n", iAudioPts, m_iAudioPts);
                   break;
               }

               //---------------------------------------------------------------

               //换算成实际时间
               //double dVideoDuration = m_pAVFrame->pkt_duration * m_dVideoTimebase;  //不用每次算浮点
               double dVideoTime = m_iVideoPts * m_dVideoTimebase;
               double dAudioTime = m_iAudioPts * m_dAudioTimebase;

               //计算同步时间
               m_dwVideoDelay = (uint32_t)m_dVideoDuration;
               if ((dVideoTime + m_dVideoDuration) < dAudioTime)
               {
                   //视频慢, 缩短帧间隔
                   m_dwVideoDelay = (m_dwVideoDelay/2) - 1;
                   LOG(Debug, "CMyFFmpeg::thread_Video()---> Video slow: (dVideoTime[%lf] + m_dVideoDuration[%lf]) < dAudioTime[%lf]; m_dwVideoDelay = (m_dVideoDuration[%lf]/2 -1) = %d; \n", dVideoTime, m_dVideoDuration, dAudioTime, m_dVideoDuration, m_dwVideoDelay);
               }
               //else if (0 < dAudioTime && (dAudioTime + m_dAudioDuration) < dVideoTime)
               else if (0 < dAudioTime && (dAudioTime + m_dAudioDuration) < dVideoTime)
               {
                   //视频快, 增加帧间隔
                   m_dwVideoDelay = m_dwVideoDelay * 1.5;  //睡眠时间
                   LOG(Debug, "CMyFFmpeg::thread_Video()---> Video fast: (dAudioTime[%lf] + m_dAudioDuration[%lf]) < dVideoTime[%lf]; m_dwVideoDelay = (m_dVideoDuration[%lf] * 1.5) = %d; \n", dAudioTime, m_dAudioDuration, dVideoTime, m_dVideoDuration, m_dwVideoDelay);
               }
               else
               {
                   //同步区间内，帧间隔不变。
                   //m_dVideoDuration = (pAVFrame->pts - m_iVideoPts) * m_dVideoTimebase;
                   //m_dwVideoDelay = (uint32_t)m_dVideoDuration;
                   LOG(Debug, "CMyFFmpeg::thread_Video()---> Video normal: dVideoTime[%lf] + m_dVideoDuration[%lf] = dAudioTime[%lf] + m_dAudioDuration[%lf]; m_dwVideoDelay = %d; \n", dVideoTime, m_dVideoDuration, dAudioTime, m_dAudioDuration, m_dwVideoDelay);
               }
           }
           while(false);
        }
        else
        {
            LOG(Debug, "CMyFFmpeg::thread_Video()---> avcodec_receive_frame() = %d; \n", iReadState);
        }

        //----------------------------------------------

        //投喂解码器
        m_MutexPacketVideo.lock();  //--加锁
        LOG(Debug, "CMyFFmpeg::thread_Video()---> m_listPacketVideo.size() = %d; \n", m_listPacketVideo.size());
        while(m_listPacketVideo.size() > 0)
        {
            //取包
            pAVPacket = m_listPacketVideo.front();

            //投喂
            iSendState = avcodec_send_packet(m_pVideoCodecCtx, pAVPacket);
            if(iSendState == AVERROR_EOF)
            {
                LOG(Debug, "CMyFFmpeg::thread_Video()---> avcodec_send_packet() = AVERROR_EOF; Set m_bClose = true; break; \n");
                m_bClose = true;
                break;
            }
            else if (iSendState == AVERROR(EAGAIN))
            {
                //解码器满
                LOG(Debug, "CMyFFmpeg::thread_Video()---> avcodec_send_packet() = AVERROR(EAGAIN); break; \n");
                break;
            }
            else //(iVideoState == 0)  //回收Packet
            {
                LOG(Debug, "CMyFFmpeg::thread_Video()---> avcodec_send_packet() = %d; \n", iSendState);

                //解引用，临时缓存Packet。
                m_listPacketVideo.pop_front();
                if(pAVPacket)
                {
                    av_packet_unref(pAVPacket);
                    listPacket.push_back(pAVPacket);
                }
            }
        }
        m_MutexPacketVideo.unlock();  //--解锁

        if(listPacket.size() > 0)
        {
            //回收Packet
            m_MutexPacketBuffer.lock();  //--加锁
            //m_listPacketBuffer.splice(m_listPacketBuffer.end(), listPacket);  //listPacket.clear();
            while (listPacket.size() > 0)
            {
                pAVPacket = listPacket.front();
                m_listPacketBuffer.push_back(pAVPacket);
                listPacket.pop_front();
            }
            m_MutexPacketBuffer.unlock();  //--解锁
        }
        else if(iReadState == AVERROR(EAGAIN))
        {
            //读不出Frame，也没Packet喂。
            m_dwVideoDelay = 40; //40ms
        }

        //--------------------------------------------------------------

        //睡一会儿
        if(m_dwVideoDelay > 0)
        {
            LOG(Debug, "CMyFFmpeg::thread_Video()---> Sleep( m_dwVideoDelay = %dms ); \n", m_dwVideoDelay);
            Sleep(m_dwVideoDelay);
        }
    }

    //释放帧缓存
    if(pAVFrame)
    {
        av_frame_free(&pAVFrame);
        pAVFrame = NULL;
    }

    if(pAVFrameYUV)
    {
        av_frame_free(&pAVFrameYUV);
        pAVFrameYUV = NULL;
    }

    LOG(Info, "CMyFFmpeg::thread_Video() End \n");
}


void CMyFFmpeg::thread_Audio()
{
    LOG(Info, "CMyFFmpeg::thread_Audio()... \n");

    if(m_pMyAudioOutput == NULL)
    {
        LOG(Error, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput = NULL; \n");
        return;
    }

    //准备AVFrames
    AVFrame *pAVFrame = av_frame_alloc();
    if (m_pAudioCodecCtx == NULL || pAVFrame == NULL)
    {
        LOG(Error, "CMyFFmpeg::thread_Audio()---> av_frame_alloc() = NULL; \n");
        return;
    }

    CMyFrame *pMyFrame = NULL;  //保存音频PCM数据
    uint8_t  *pMyData  = NULL;

    std::list<AVPacket*> listPacket;
    AVPacket *pAVPacket = NULL;
    int iAudioState = 0;

    int iAudioFrameData = 2048;
    int iAudioFrameCount = 16;

    while (!m_bClose)
    {
        //暂停播放
//         if (m_bPause)
//         {
//             LOG(Debug, "CMyFFmpeg::thread_Audio()---> m_bPause = true; Sleep(200); \n");
//             Sleep(200);
//             continue;
//         }

        //----------------------------------------------

        //从解码器读取Frame
        iAudioState = avcodec_receive_frame(m_pAudioCodecCtx, pAVFrame);
        if(iAudioState == AVERROR_EOF)  // --- 解码器读尽，播放线程结束。---
        {
            LOG(Info, "CMyFFmpeg::thread_Audio()---> avcodec_receive_frame() = AVERROR_EOF; m_pMyAudioOutput->updatePlayState(enClose=%d); break; \n", enClose);
            m_pMyAudioOutput->updatePlayState(enClose);
            break;
        }
        else if(iAudioState == AVERROR(EINVAL))  //codec not opened, or it is an encoder
        {
            LOG(Error, "CMyFFmpeg::thread_Audio()---> avcodec_receive_frame() = AVERROR(EINVAL); break; \n");
            break;
        }
        else if (iAudioState == 0)  //读取到数据
        {
            LOG(Debug, "CMyFFmpeg::thread_Audio()---> avcodec_receive_frame() = 0; \n");

            LOG(Debug, "CMyFFmpeg::thread_Audio()---> AVFrame[Audio]: best_effort_timestamp=%lld, pts=%lld, format=%d, key_frame=%d, pict_type=%d, width=%d, height=%d; "
                        "pkt_pos=%lld, pkt_dts=%lld, pkt_duration=%lld, pkt_size=%d; "
                        "coded_picture_number=%d, display_picture_number=%d, repeat_pict=%d, interlaced_frame=%d; top_field_first=%d, flag=%d; "
                        "sample_rate=%d, nb_samples=%d, channels=%d, channel_layout=%d; \n",

                        pAVFrame->best_effort_timestamp,
                        pAVFrame->pts,
                        pAVFrame->format,

                        pAVFrame->key_frame,
                        pAVFrame->pict_type,
                        pAVFrame->width,
                        pAVFrame->height,

                        pAVFrame->pkt_pos,
                        pAVFrame->pkt_dts,
                        pAVFrame->pkt_duration,
                        pAVFrame->pkt_size,

                        pAVFrame->coded_picture_number,
                        pAVFrame->display_picture_number,
                        pAVFrame->repeat_pict,
                        pAVFrame->interlaced_frame,
                        pAVFrame->top_field_first,
                        pAVFrame->flags,

                        pAVFrame->sample_rate,
                        pAVFrame->nb_samples,
                        pAVFrame->channels,
                        pAVFrame->channel_layout);

            //----------------------------------------------

            if (m_pAudioConvertCtx == NULL)
            {
                //准备音频转换器
                //m_pAudioConvertCtx = swr_alloc();
                m_pAudioConvertCtx = swr_alloc_set_opts(NULL,
                                                        AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, m_pAudioCodecCtx->sample_rate,
                                                        m_pAudioCodecCtx->channel_layout,
                                                        m_pAudioCodecCtx->sample_fmt,
                                                        m_pAudioCodecCtx->sample_rate,
                                                        0, NULL);

                if (m_pAudioConvertCtx == NULL)
                {
                    LOG(Error, "CMyFFmpeg::thread_Audio()---> swr_alloc_set_opts() Fail; break; \n");
                    break;
                }

                int iRet = swr_init(m_pAudioConvertCtx);
                if (iRet != 0)
                {
                    LOG(Error, "CMyFFmpeg::thread_Audio()---> swr_init() = %d; break; \n", iRet);
                    break;
                }

                //SDL_OpenAudioDevice(NULL, 0, ...);
                //SDL_GetAudioDeviceName(SDL_GetNumAudioDevices(0), 0);
                //SDL_PauseAudio(0);

                //计算音频缓存区大小
                iAudioFrameData = av_samples_get_buffer_size(NULL, 2, pAVFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
                LOG(Info, "CMyFFmpeg::thread_Audio()---> av_samples_get_buffer_size(iChannel=2, nb_samples=%d, AV_SAMPLE_FMT_S16, align=1) = %d; \n", pAVFrame->nb_samples, iAudioFrameData);

                //设置音频缓存区
                LOG(Info, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->setFrameBuffer(iFrameData=%d, iFrameCount=%d); \n", iAudioFrameData, iAudioFrameCount);
                iRet = m_pMyAudioOutput->setAudioBuffer(iAudioFrameData, iAudioFrameCount);  //iFrameCount=16;
                if(iRet < 0)
                {
                    LOG(Error, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->setAudioBuffer() = %d; break; \n", iRet);
                    break;
                }

                //设置音频播放格式
                LOG(Info, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->setAudioFormat(iChannel=2, iSampleRate=%d, iSampleFormat=%d); \n", pAVFrame->sample_rate, pAVFrame->format);
                iRet = m_pMyAudioOutput->setAudioFormat(2, pAVFrame->sample_rate, pAVFrame->format);
                if(iRet < 0)
                {
                    LOG(Error, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->setAudioFormat() = %d; break; \n", iRet);
                    break;
                }

                //启动音频播放
                LOG(Info, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->startAudioOutput(); \n");
                m_pMyAudioOutput->startAudioOutput();
            }

            //----------------------------------------------

            //取PCM音频缓存
            while(pMyFrame == NULL && m_bClose == false)
            {
                pMyFrame = m_pMyAudioOutput->popFree();
                if(pMyFrame == NULL)
                {
                    //计算睡眠时间
                    int64_t iBufferDuration = m_pMyAudioOutput->getBufferDuration();
                    int iBufferDelay = (int)((iBufferDuration - m_iAudioDuration) * m_dAudioTimebase * 0.5) - 10;  //少睡10ms
                    if (iBufferDelay > 10)
                    {
                        //LOG(Debug, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->popFree() = NULL; m_pMyAudioOutput->getBufferDuration()=%d; m_iAudioDuration=%d; m_dAudioTimebase[%lf]; Sleep(%dms); \n", iBufferDuration, m_iAudioDuration, m_dAudioTimebase, iBufferDelay);
                        Sleep(iBufferDelay);
                    }
                    else
                    {
                        iBufferDelay = 20;
                        LOG(Warn, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->popFree() = NULL; m_pMyAudioOutput->getBufferDuration()=%d; m_iAudioDuration=%d; m_dAudioTimebase[%lf]; Sleep(%dms); \n", iBufferDuration, m_iAudioDuration, m_dAudioTimebase, iBufferDelay);
                        Sleep(iBufferDelay);
                    }

                    continue;
                }

                pMyData = pMyFrame->data;
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->popFree() = %p; pMyData = %p; pMyFrame->iData = %d; \n", pMyFrame, pMyData, pMyFrame->iData);
            }

            if(pMyFrame && pMyData)
            {
                // 转换音频
                int iRet = swr_convert(m_pAudioConvertCtx, &pMyData, iAudioFrameData, (const uint8_t **)pAVFrame->data, pAVFrame->nb_samples);
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> swr_convert( iAudioFrameData=%d, pAVFrame->nb_samples=%d ) = %d; \n", iAudioFrameData, pAVFrame->nb_samples, iRet);

                //拷贝时间戳
                pMyFrame->pts = pAVFrame->pts;
                pMyFrame->dts = pAVFrame->pkt_dts;
                pMyFrame->duration = pAVFrame->pkt_duration;
                pMyFrame->iData = iAudioFrameData;

                //投入音频对象
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->pushWork(pMyFrame); \n");
                m_pMyAudioOutput->pushWork(pMyFrame);
                pMyFrame = NULL;
                pMyData  = NULL;
            }

            continue;
        }
        //else (iAudioState == AVERROR(EAGAIN)  //读尽了
        else
        {
            LOG(Debug, "CMyFFmpeg::thread_Audio()---> avcodec_receive_frame() = %d; \n", iAudioState);
        }

        //----------------------------------------------

        //投喂解码器
        m_MutexPacketAudio.lock();  //--加锁
        LOG(Debug, "CMyFFmpeg::thread_Audio()---> m_listPacketAudio.size() = %d; \n", m_listPacketAudio.size());
        while(m_listPacketAudio.size() > 0)
        {
            //取包
            pAVPacket = m_listPacketAudio.front();

            //投喂
            iAudioState = avcodec_send_packet(m_pAudioCodecCtx, pAVPacket);
            if(iAudioState == AVERROR_EOF)
            {
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> avcodec_send_packet() = AVERROR_EOF; \n");
                break;
            }
            else if (iAudioState == AVERROR(EAGAIN))
            {
                //解码器满
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> avcodec_send_packet() = AVERROR(EAGAIN); \n");
                break;
            }
            else //(iVideoState == 0)  //回收Packet
            {
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> avcodec_send_packet() = 0; \n");

                //解引用，临时缓存Packet。
                m_listPacketAudio.pop_front();
                if(pAVPacket)
                {
                    av_packet_unref(pAVPacket);
                    listPacket.push_back(pAVPacket);
                }
            }
        }
        m_MutexPacketAudio.unlock();  //--解锁

        //如果没有数据喂解码器，睡一会儿。。。
        if(listPacket.size() == 0)
        {
            int iBufferDelay = 0;

            //计算睡眠时间
            int64_t iBufferDuration = m_pMyAudioOutput->getBufferDuration();
            if (iBufferDuration > m_iAudioDuration)
            {
                iBufferDelay = (int)((iBufferDuration - m_iAudioDuration) * m_dAudioTimebase) - 10;  //少睡10ms
                if(iBufferDelay < 10)
                {
                    iBufferDelay = 10;
                }
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> listPacket.size() = 0; m_pMyAudioOutput->getBufferDuration()=%d; m_iAudioDuration=%d; m_dAudioTimebase[%lf]; Sleep(%dms); \n", iBufferDuration, m_iAudioDuration, m_dAudioTimebase, iBufferDelay);
                
            }
            else if (m_dwAudioDelay > 0)
            {
                iBufferDelay = 10;  //睡10ms
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> listPacket.size() = 0; m_pMyAudioOutput->getBufferDuration()=%d; m_dwAudioDelay=%d; Sleep(%dms); \n", iBufferDuration, m_dwAudioDelay, iBufferDelay);
            }
            else
            {
                iBufferDelay = 200;
                LOG(Debug, "CMyFFmpeg::thread_Audio()---> listPacket.size() = 0; m_pMyAudioOutput->getBufferDuration()=%d; m_dwAudioDelay=%d; Sleep(%dms); \n", iBufferDuration, m_dwAudioDelay, iBufferDelay);
            }

            //睡一会儿
            if (iBufferDelay >= 10)
            {
                Sleep(iBufferDelay);
            }
        }
        else
        {
            //回收Packet
            m_MutexPacketBuffer.lock();  //--加锁
            //m_listPacketBuffer.splice(m_listPacketBuffer.end(), listPacket);  //listPacket.clear();
            while (listPacket.size() > 0)
            {
                pAVPacket = listPacket.front();
                m_listPacketBuffer.push_back(pAVPacket);
                listPacket.pop_front();
            }
            m_MutexPacketBuffer.unlock();  //--解锁
        }
    }

    //取声音缓存
    int64_t iBufferDuration = m_pMyAudioOutput->getBufferDuration();
    if (iBufferDuration > 0)
    {
        int iBufferDelay = (int)((iBufferDuration + m_iAudioDuration) * m_dAudioTimebase * 0.5);
        LOG(Info, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->getBufferDuration()=%d; m_iAudioDuration=%d; m_dAudioTimebase[%lf]; Sleep(%dms); \n", iBufferDuration, m_iAudioDuration, m_dAudioTimebase, iBufferDelay);
        if (iBufferDelay > 0)
        {
            Sleep(iBufferDelay);
        }
    }

    //停止播放
    LOG(Info, "CMyFFmpeg::thread_Audio()---> m_pMyAudioOutput->stopAudioOutput(); \n");
    m_pMyAudioOutput->stopAudioOutput();

    //归还
    if(pMyFrame)
    {
        m_pMyAudioOutput->pushFree(pMyFrame);
    }

    //释放内存
    if (pAVFrame)
    {
        av_frame_free(&pAVFrame);
        //swr_free(&m_pAudioConvertCtx);
    }

    LOG(Info, "CMyFFmpeg::thread_Audio() End \n");
}
