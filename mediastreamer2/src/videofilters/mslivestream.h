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
#ifndef __MS_LIVE_STREAM_H__
#define __MS_LIVE_STREAM_H__

#include "mediastreamer2/msvideo.h"


// 输出流类型
#define CAM_LIVE_TYPE_NONE        0x0
#define CAM_LIVE_TYPE_YUV420P     0x1
#define CAM_LIVE_TYPE_H264        0x2
#define CAM_LIVE_TYPE_H265        0x4

typedef void(*CB_Live_StreamData)(int dataType, YuvBuf *yuv420p, void* userData);

class CamStream
{
public:
	CamStream() {}
	virtual ~CamStream() {}

public:
	/*
		参数：
			uri  摄像机流描述字符串
			RTSP流        rtsp://admin:pass@19.0.0.215:554/H264?ch=1&subtype=1
			callback      回调输出流
	*/
	virtual bool open(const char *addr) = 0;
	virtual void Close() = 0;
	virtual bool read_stream(CB_Live_StreamData callback, void *userData, bool decode) = 0;

	virtual unsigned int support_fmts() { return 0x0; }
};

CamStream* create_CamStream(const char *_url);

#endif // __MS_LIVE_STREAM_H__