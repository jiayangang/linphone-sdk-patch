/*****************************************************************************

					版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : h264pack.cc
	作者     : 贾延刚
	创建日期 : 2020-11-04

	版本     : 1.0
	功能描述 :
			   把H264流按照rfc3984打包

	说明     ：
	           解析H264流，去掉每一帧前边的 00 00 00 01
	           然后按照rfc3984打包
	   
******************************************************************************/
#include "h264pack.h"

h264_pack::h264_pack()
{
	m_msq = NULL;
	m_cur_ts = 0;
	m_cseq = 0;
}
h264_pack::~h264_pack(){}

void h264_pack::cbNaluPacket(int nalu_type, unsigned char *buffer, int data_len)
{
	pack_src_t pack_src;

	pack_src.buf = (char*)buffer;
	pack_src.len = data_len;
	pack_src.forbidden_bit     = pack_src.buf[0] & 0x80; //1 bit
	pack_src.nal_reference_idc = pack_src.buf[0] & 0x60; // 2 bit
	pack_src.nal_unit_type     = pack_src.buf[0] & 0x1f; // 5 bit

	pack_src.cseq = this->m_cseq;


	h2642fragment(&pack_src, m_msq, m_cur_ts);
	this->m_cseq = pack_src.cseq;
}

void h264_pack::feed(mblk_t *m, MSQueue *rtpq, uint32_t ts)
{
	m_msq = rtpq;
	m_cur_ts = ts;

	m_parser.Parse(m->b_rptr, (m->b_wptr - m->b_rptr));
}

void h264_pack::initialize()
{
	m_parser.Init(this);
}