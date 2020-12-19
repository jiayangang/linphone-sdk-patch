/*****************************************************************************

					版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : h264fragment.h
	作者     : 贾延刚
	创建日期 : 2020-11-04

	版本     : 1.0
	功能描述 :
	           把H264帧按照rfc3984分片

	说明     ：
	           

******************************************************************************/
#ifndef __H264_FRAGMENT_H__
#define __H264_FRAGMENT_H__

#include <mediastreamer2/msqueue.h>

typedef struct
{
	char *buf;                    //! contains the first byte followed by the EBSP
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)

	int forbidden_bit;            //! should be always FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx    

	uint32_t  cseq;
} pack_src_t;

int h2642fragment(pack_src_t *n, MSQueue *rtpq, uint32_t ts);

#endif // __H264_FRAGMENT_H__