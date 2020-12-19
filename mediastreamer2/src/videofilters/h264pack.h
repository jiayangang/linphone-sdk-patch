/*****************************************************************************

					版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : msh264pack.h
	作者     : 贾延刚
	创建日期 : 2019-03-19

	版本     : 1.0
	功能描述 :
	           把H264流按照rfc3984打包

	说明     ：
	           解析H264流，去掉每一帧前边的 00 00 00 01
	           然后按照rfc3984打包

******************************************************************************/
#ifndef __H264_PACK_H__
#define __H264_PACK_H__

#include "nalu_parser.h"
#include "h264fragment.h"
#include <mediastreamer2/msqueue.h>

class h264_pack : public I_nalu_parser
{
public:
	h264_pack();
	~h264_pack();

public:
	void initialize();	
	void feed(mblk_t *im, MSQueue *rtpq, uint32_t ts);

private:
	nalu_parser  m_parser;
	MSQueue     *m_msq;
	uint32_t     m_cur_ts;
	uint32_t     m_cseq;
	
	virtual void cbNaluPacket(int nalu_type, unsigned char *buffer, int data_len);
};
#endif // __H264_PACK_H__