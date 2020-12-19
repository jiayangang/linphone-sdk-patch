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