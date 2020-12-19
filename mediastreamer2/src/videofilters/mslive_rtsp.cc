/*****************************************************************************

					版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : mslive_rtsp.cpp
	作者     : 贾延刚
	创建日期 : 2020-11-03

	版本     : 1.0
	功能描述 :
	来自RTSP服务的实时流

	说明     ：


******************************************************************************/
#include "mslive_rtsp.h"

CExitMark::CExitMark()
{
	ms_mutex_init(&_mutex, NULL);
	m_bExit = false;
}

CExitMark::~CExitMark()
{
	ms_mutex_destroy(&_mutex);
}

bool CExitMark::IsToExit()
{
	ms_mutex_lock(&_mutex);
	bool result = m_bExit;
	ms_mutex_unlock(&_mutex);
	return result;
}
void CExitMark::SetExit()
{
	ms_mutex_lock(&_mutex);
	m_bExit = true;
	ms_mutex_unlock(&_mutex);
}

// 单位毫秒
class timeout_handler
{
public:
	timeout_handler(int64_t TimeoutMs)
		:timeout_ms_(TimeoutMs)
	{}

	void reset(int64_t TimeoutMs)
	{
		timeout_ms_ = TimeoutMs;
		lastTime_ = av_gettime() / 1000;
	}

	bool is_timeout()
	{
		int64_t actualDelay = av_gettime() / 1000 - lastTime_;
		return actualDelay > timeout_ms_;
	}

	// 返回1，阻塞结束
	static int check_interrupt(void * t)
	{
		int result = (t && static_cast<timeout_handler *>(t)->is_timeout()) ? 1 : 0;
		return result;
	}

public:
	int64_t timeout_ms_;
	int64_t lastTime_;
};


CamRTSP::CamRTSP()
{
	m_callback = NULL;
	m_userData = NULL;
	m_bDecode = false;

	m_st_video = NULL;
	m_ixVideo = -1;
	m_formatCtx = NULL;
	m_th = NULL;
	m_frame = NULL;
}

CamRTSP::~CamRTSP()
{
}

void *CamRTSP::ThreadFunc(void *lpParam)
{
	CamRTSP *l = (CamRTSP *)lpParam;
	l->Run();
	return NULL;
}
void CamRTSP::Terminate()
{
}

bool CamRTSP::WaitTimeout(DWORD dwMilliseconds)
{
	if (m_thread)
		ms_thread_join(m_thread, NULL);

	return true;
}

bool CamRTSP::Start()
{
	ms_thread_create(&m_thread, NULL, ThreadFunc, this);
	return true;
}

bool CamRTSP::ReadPacket(AVPacket *packet)
{
	if (m_th) m_th->reset(m_timeout);
	if (av_read_frame(m_formatCtx, packet) >= 0)
	{
		if (packet->stream_index == m_ixVideo)
		{
			return true;
		}
		else
		{
#ifdef _WIN32
			av_packet_unref(packet);   // TODO: 需要检查
#endif
			av_free_packet(packet);
		}
	}
	else
	{
		if (m_formatCtx->pb && m_formatCtx->pb->error)
		{
			ms_usleep(10*1000);
		}
	}
	return false;
}

void CamRTSP::Run()
{
	YuvBuf yuv420p;
	while (1)
	{
		if (m_exitMark.IsToExit())
			break;

		AVPacket *src_pkt = &m_packet;
		bool b = ReadPacket(src_pkt);
		if (b)
		{
			if (m_bDecode) 
			{
				AVFrame *picture = m_frame;
				int got_picture;
				int ret = avcodec_decode_video2(m_st_video->codec, picture, &got_picture, src_pkt);  // 关闭时，会异常
				if (ret < 0)
				{
					ms_error("Decode Error.\n");
					continue;
				}

				if (got_picture)
				{
					yuv420p.w = m_st_video->codec->width;
					yuv420p.h = m_st_video->codec->height;
					yuv420p.planes[0] = picture->data[0];
					yuv420p.planes[1] = picture->data[1];
					yuv420p.planes[2] = picture->data[2];
					yuv420p.strides[0] = picture->linesize[0];
					yuv420p.strides[1] = picture->linesize[1];
					yuv420p.strides[2] = picture->linesize[2];
					if (m_callback) m_callback(CAM_LIVE_TYPE_YUV420P, &yuv420p, m_userData);
				}
			}
			else {
				yuv420p.w = m_st_video->codec->width;
				yuv420p.h = m_st_video->codec->height;
				yuv420p.strides[0] = src_pkt->size;
				yuv420p.planes[0] = src_pkt->data;
				if (m_callback) m_callback(CAM_LIVE_TYPE_H264, &yuv420p, m_userData);
			}
		}
	}
}

void CamRTSP::Close()
{
	m_exitMark.SetExit();
	if (!WaitTimeout(1000)) //TODO : 异常
	{
		Terminate();
	}
	CloseRtsp();
}



/*
timeout 等待时间，毫秒
*/
bool CamRTSP::OpenRtspAddr2(const char *url, int timeout)
{
	if (!url || *url == '\0')
		return false;

	av_register_all();
	avformat_network_init();
	m_formatCtx = avformat_alloc_context();

	if (!m_th) m_th = new timeout_handler(timeout);
	m_formatCtx->interrupt_callback.callback = &timeout_handler::check_interrupt;
	m_formatCtx->interrupt_callback.opaque = (void*)m_th;
	//AVFMT_FLAG_NONBLOCK
	m_th->reset(timeout);
	m_timeout = timeout;

	AVDictionary *options = NULL;
	av_dict_set(&options, "analyzeduration", "1000000", 0);  // 单位微秒
	av_dict_set(&options, "probesize", "100000", 0);  // 100 * 1000
	av_dict_set(&options, "buffer_size", "1024000", 0);
	av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间 毫秒  max_delay  2020-7-16 该项之前是禁用的，打开后rtmp才正常播放
	if (avformat_open_input(&m_formatCtx, url, NULL, &options) != 0)
		//if (avformat_open_input(&m_formatCtx, url, NULL, &options) != 0)
	{
		av_dict_free(&options);
		ms_error("Couldn't open input stream.\n");
		return false;
	}
	av_dict_free(&options);

	m_formatCtx->max_analyze_duration = 1000;
	//m_formatCtx->max_analyze_duration2 = 1000;
	if (avformat_find_stream_info(m_formatCtx, NULL) < 0)
	{
		ms_error("Couldn't find stream information.\n");
		return false;
	}

	int ixVideo = -1;
	for (int k = 0; k< (int)m_formatCtx->nb_streams; k++)
		if (m_formatCtx->streams[k]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			ixVideo = k;
			m_st_video = m_formatCtx->streams[k];
			break;
		}
	if (ixVideo == -1)
	{
		ms_error("Didn't find a video stream.\n");
		return false;
	}

	//av_dump_format(m_formatCtx, 0, url, 0);
	m_ixVideo = ixVideo;
	m_st_video = m_formatCtx->streams[ixVideo];
	return true;
}

unsigned int CamRTSP::support_fmts()
{
	unsigned int result = CAM_LIVE_TYPE_YUV420P;
	if (0 == strncmp(m_url, "rtsp", 4))
	{	
		if (AV_CODEC_ID_H264 == m_st_video->codec->codec_id)
			result |= CAM_LIVE_TYPE_H264;
	}
	
	return result;
}

bool CamRTSP::read_stream(CB_Live_StreamData callback, void *userData, bool decode)
{
	m_callback = callback;
	m_userData = userData;

	m_bDecode = decode;

	if (m_bDecode)
	{
		AVStream *st = m_formatCtx->streams[m_ixVideo];
		AVCodec *codec = avcodec_find_decoder(st->codec->codec_id);
		if (!codec)
			return false;

		if (avcodec_open2(st->codec, codec, NULL) != 0)
			return false;

		m_frame = av_frame_alloc();// avcodec_alloc_frame();
		m_st_video = st;
	}
	this->Start();
	return true;
}

bool CamRTSP::CloseRtsp()
{
	if (m_formatCtx)
	{
		if (m_st_video) avcodec_close(m_st_video->codec);
		avformat_close_input(&m_formatCtx);
		m_formatCtx = NULL;
	}

	if (m_frame) av_frame_free(&m_frame);
	m_frame = NULL;
	if (m_th) delete m_th;
	m_th = NULL;
	return true;
}

bool CamRTSP::open(const char *addr)
{
	strncpy(m_url, addr, 255);
	return OpenRtspAddr2(addr, 10 * 1000);
}