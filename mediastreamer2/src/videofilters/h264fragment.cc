/*****************************************************************************

					版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : h264fragment.cc
	作者     : 贾延刚
	创建日期 : 2020-11-04

	版本     : 1.0
	功能描述 :
	           把H264帧按照rfc3984分片

	说明     ：
	           

******************************************************************************/
#include "h264fragment.h"


#define  UDP_MAX_SIZE          1400
#define MAX_RTP_PKT_LENGTH     1400



/******************************************************************
NALU_HEADER
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
******************************************************************/
typedef struct {
	//byte 0
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} NALU_HEADER; /* 1 byte */


			   /******************************************************************
			   FU_INDICATOR
			   +---------------+
			   |0|1|2|3|4|5|6|7|
			   +-+-+-+-+-+-+-+-+
			   |F|NRI|  Type   |
			   +---------------+
			   ******************************************************************/
typedef struct {
	//byte 0
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} FU_INDICATOR; /*1 byte */


				/******************************************************************
				FU_HEADER
				+---------------+
				|0|1|2|3|4|5|6|7|
				+-+-+-+-+-+-+-+-+
				|S|E|R|  Type   |
				+---------------+
				******************************************************************/
typedef struct {
	//byte 0
	unsigned char TYPE : 5;
	unsigned char R : 1;
	unsigned char E : 1;
	unsigned char S : 1;
} FU_HEADER; /* 1 byte */


float        framerate = 15;
unsigned int timestamp_increse = (unsigned int)(90000.0 / framerate); //+0.5);
unsigned int ts_current = 0;

static NALU_HEADER		*nalu_hdr;
static FU_INDICATOR	    *fu_ind;
static FU_HEADER		*fu_hdr;

int h2642fragment(pack_src_t *n, MSQueue *rtpq, uint32_t ts)
{
	//清空buf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值

	//当一个NALU小于1400字节的时候，采用一个单RTP包发送
	if (n->len <= UDP_MAX_SIZE)
	{
		// 时间戳
		ts_current = ts_current + timestamp_increse;

		//im->b_wptr = im->b_rptr;  // 恢复写入位置
		mblk_t *im = allocb(1500, 0);


		// 0 设置NALU HEADER
		nalu_hdr = (NALU_HEADER*)im->b_wptr; // 地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入buf中；
		nalu_hdr->F = n->forbidden_bit;
		nalu_hdr->NRI = n->nal_reference_idc >> 5;//有效数据在n->nal_reference_idc的第6，7位，需要右移5位才能将其值赋给nalu_hdr->NRI。
		nalu_hdr->TYPE = n->nal_unit_type;
		im->b_wptr += 1;

		// 1
		memcpy(im->b_wptr, n->buf + 1, n->len - 1);  // 去掉nalu头的nalu剩余内容写入
		im->b_wptr += n->len - 1;

		//设置、用于rtp M 位
		mblk_set_marker_info(im, 1);

		mblk_set_timestamp_info(im, ts);
		mblk_set_cseq(im, n->cseq++);
		ms_queue_put(rtpq, im);
		return 1;
		//m_channel->sendm_with_ts(im, ts_current); //发送RTP包
		//Sleep(100);
		//freemsg(im);
	}
	else  // 分片
	{
		int packetNum = n->len / UDP_MAX_SIZE;
		if (n->len % UDP_MAX_SIZE != 0)
			packetNum++;

		int lastPackSize = n->len - (packetNum - 1)*UDP_MAX_SIZE;  // 最后一个包的大小
		int packetIndex = 1;

		ts_current = ts_current + timestamp_increse;   // 同一个包的分片使用同一个时间戳


		mblk_t *im = allocb(1500, 0);
		//im->b_wptr = im->b_rptr;  // 恢复写入位置


		//发送第一个的FU，S=1，E=0，R=0

		//设置rtp M 位；
		mblk_set_marker_info(im, 0);



		// 0 设置FU INDICATOR
		fu_ind = (FU_INDICATOR*)im->b_wptr;  // 地址赋给fu_ind，之后对fu_ind的写入就将写入buf中；
		fu_ind->F = n->forbidden_bit;
		fu_ind->NRI = n->nal_reference_idc >> 5;
		fu_ind->TYPE = 28;

		im->b_wptr += 1;

		// 1 设置FU HEADER 
		fu_hdr = (FU_HEADER*)im->b_wptr;
		fu_hdr->S = 1;
		fu_hdr->E = 0;
		fu_hdr->R = 0;
		fu_hdr->TYPE = n->nal_unit_type;

		// 2  数据
		im->b_wptr += 1;
		memcpy(im->b_wptr, n->buf + 1, UDP_MAX_SIZE);  // 去掉NALU头
		im->b_wptr += UDP_MAX_SIZE;


		mblk_set_timestamp_info(im, ts);
		mblk_set_cseq(im, n->cseq++);
		ms_queue_put(rtpq, im);


		//m_channel->sendm_with_ts(im, ts_current); //发送RTP包
		//freemsg(im);

		//发送中间的FU，S=0，E=0，R=0
		for (packetIndex = 2; packetIndex<packetNum; packetIndex++)
		{
			//设置rtp M 位
			mblk_set_marker_info(im, 0);


			mblk_t *im = allocb(1500, 0);
			//im->b_wptr = im->b_rptr;  // 恢复写入位置


			// 0  设置FU INDICATOR
			fu_ind = (FU_INDICATOR*)im->b_wptr;// 将地址赋给fu_ind，之后对fu_ind的写入就将写入buf中；
			fu_ind->F = n->forbidden_bit;
			fu_ind->NRI = n->nal_reference_idc >> 5;
			fu_ind->TYPE = 28;

			im->b_wptr += 1;

			// 1  设置FU HEADER
			fu_hdr = (FU_HEADER*)im->b_wptr;
			fu_hdr->S = 0;
			fu_hdr->E = 0;
			fu_hdr->R = 0;
			fu_hdr->TYPE = n->nal_unit_type;

			im->b_wptr += 1;

			// 2  数据
			memcpy(im->b_wptr, n->buf + (packetIndex - 1)*UDP_MAX_SIZE + 1, UDP_MAX_SIZE);   //去掉起始前缀的nalu剩余内容写入
			im->b_wptr += UDP_MAX_SIZE;


			//memcpy(nalu_payload, n->buf+(packetIndex-1)*UDP_MAX_SIZE+1, UDP_MAX_SIZE); //去掉起始前缀的nalu剩余内容写入buf[14]开始的字符串。
			//bytes = UDP_MAX_SIZE+2;//14; //获得buf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节

			mblk_set_timestamp_info(im, ts);
			mblk_set_cseq(im, n->cseq++);
			ms_queue_put(rtpq, im);
			//m_channel->sendm_with_ts(im, ts_current); //发送rtp包	
			//freemsg(im);
		}

		//发送最后一个的FU，S=0，E=1，R=0


		im = allocb(1500, 0);
		// im->b_wptr = im->b_rptr;  // 恢复写入位置

		//设置rtp M 位；当前传输的是最后一个分片时该位置1  
		mblk_set_marker_info(im, 1);




		// 0 设置FU INDICATOR
		fu_ind = (FU_INDICATOR*)im->b_wptr;// 将地址赋给fu_ind，之后对fu_ind的写入就将写入buf中；
		fu_ind->F = n->forbidden_bit;
		fu_ind->NRI = n->nal_reference_idc >> 5;
		fu_ind->TYPE = 28;

		im->b_wptr += 1;
		// 1 设置FU HEADER
		fu_hdr = (FU_HEADER*)im->b_wptr;
		fu_hdr->S = 0;
		fu_hdr->E = 1;
		fu_hdr->R = 0;
		fu_hdr->TYPE = n->nal_unit_type;

		// 2
		im->b_wptr += 1;
		memcpy(im->b_wptr, n->buf + (packetIndex - 1)*UDP_MAX_SIZE + 1, lastPackSize - 1);
		im->b_wptr += lastPackSize - 1;

		mblk_set_timestamp_info(im, ts);
		mblk_set_cseq(im, n->cseq++);
		ms_queue_put(rtpq, im);
		return packetNum;

		//memcpy(nalu_payload,n->buf+(packetIndex-1)*UDP_MAX_SIZE+1,lastPackSize-1);//将nalu最后剩余的-1(去掉了一个字节的NALU头)字节内容写入buf[14]开始的字符串。
		//bytes = lastPackSize-1+2;//14;//获得buf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节
		//m_channel->sendm_with_ts(im, ts_current); //发送rtp包	
		//freemsg(im);
	}
	//freemsg(im);
}
