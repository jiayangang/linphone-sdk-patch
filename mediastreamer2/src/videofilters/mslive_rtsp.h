/*****************************************************************************

					��Ȩ���� (C), 2001-2050,

******************************************************************************
	�ļ����� : mslive_rtsp.h
	����     : ���Ӹ�
	�������� : 2020-11-03

	�汾     : 1.0
	�������� :
	           ����RTSP�����ʵʱ��

	˵��     ��


******************************************************************************/
#ifndef __MS_LIVE_RTSP_H__
#define __MS_LIVE_RTSP_H__

#include "mslivestream.h"


#ifdef _WIN32
#ifdef __cplusplus
/*see http://linux.die.net/man/3/uint64_c */
#define __STDC_CONSTANT_MACROS 1
#endif

#define UINT64_C(val) val##ui64


#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#ifdef   __cplusplus
}
#endif

#else
#include "ffmpeg-priv.h"
typedef unsigned int DWORD;
#define AV_CODEC_ID_H264     CODEC_ID_H264
#endif


class CExitMark
{
public:
	CExitMark();
	~CExitMark();
public:
	bool  IsToExit();
	void  SetExit();

private:
	ms_mutex_t   _mutex;
	bool          m_bExit;
};

class timeout_handler;
class CamRTSP : public CamStream
{
public:
	CamRTSP();
	virtual ~CamRTSP();

public:
	virtual bool open(const char *addr);
	virtual void Close();
	virtual bool read_stream(CB_Live_StreamData callback, void *userData, bool decode);
	virtual unsigned int support_fmts();
	

private:
	CExitMark           m_exitMark;
	CB_Live_StreamData  m_callback;
	void               *m_userData;
	ms_thread_t         m_thread;   /* the thread ressource*/

	static void *ThreadFunc(void *lpParam);
	virtual void Run();
	bool Start();
	bool WaitTimeout(DWORD dwMilliseconds);
	void Terminate();

private:
	bool              m_bDecode;
	timeout_handler  *m_th;
	int               m_timeout;

	AVFormatContext	 *m_formatCtx;
	int               m_ixVideo;
	AVPacket          m_packet;

	bool CloseRtsp();
	bool ReadPacket(AVPacket *packet);

	bool OpenRtspAddr2(const char *url, int timeout);

private:
	AVStream         *m_st_video;
	AVFrame          *m_frame;
	char              m_url[256];

};

#endif // __MS_LIVE_RTSP_H__
