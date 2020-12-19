/*****************************************************************************

					版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : msipcstream.h
	作者     : 贾延刚
	创建日期 : 2019-03-18

	版本     : 1.0
	功能描述 :
			   获取IPC摄像机的实时流

	说明     ：


******************************************************************************/
#include "mslive_rtsp.h"
#ifdef _WIN32
#include "mslive_zkth.h"
#endif // _WIN32

CamStream* create_CamStream(const char *_url)
{
	if (!_url || *_url == '\0')
	{
		ms_error("%s _url is blank.", __FUNCTION__);
		return NULL;
	}

	CamStream *m_camStream = NULL;
	if (0 == strncmp(_url, "rtsp", 4) ||
		0 == strncmp(_url, "rtmp", 4))
	{
		m_camStream = new CamRTSP();
	}
#ifdef _WIN32
	else {
		m_camStream = new CamZKTH();
	}
#endif // _WIN32

	return m_camStream;
}