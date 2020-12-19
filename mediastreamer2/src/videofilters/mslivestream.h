/*****************************************************************************

					��Ȩ���� (C), 2001-2050,

******************************************************************************
	�ļ����� : msipcstream.h
	����     : ���Ӹ�
	�������� : 2019-03-18

	�汾     : 1.0
	�������� :
	           ��ȡIPC�������ʵʱ��

	˵��     ��


******************************************************************************/
#ifndef __MS_LIVE_STREAM_H__
#define __MS_LIVE_STREAM_H__

#include "mediastreamer2/msvideo.h"


// ���������
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
		������
			uri  ������������ַ���
			RTSP��        rtsp://admin:pass@19.0.0.215:554/H264?ch=1&subtype=1
			callback      �ص������
	*/
	virtual bool open(const char *addr) = 0;
	virtual void Close() = 0;
	virtual bool read_stream(CB_Live_StreamData callback, void *userData, bool decode) = 0;

	virtual unsigned int support_fmts() { return 0x0; }
};

CamStream* create_CamStream(const char *_url);

#endif // __MS_LIVE_STREAM_H__