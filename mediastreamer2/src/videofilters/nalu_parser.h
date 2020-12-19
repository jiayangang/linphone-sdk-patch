/*****************************************************************************

				版权所有 (C), 2001-2050,

******************************************************************************
	文件名称 : nalu_parser.h
	作者     : 贾延刚 
	创建日期 : 2020-11-04

	版本     : 1.0
	功能描述 : 
	           从实时流中获取NALU。

	说明     ：


******************************************************************************/
#ifndef __NALU_PARSER_H__
#define __NALU_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class I_nalu_parser
{
public:
	virtual void cbNaluPacket(int nalu_type, unsigned char *buffer, int data_len) = 0;
};

class nalu_parser
{
public:
	nalu_parser(void);
	virtual ~nalu_parser(void) {}

public:
	bool  Init(I_nalu_parser *l)
	{
		m_listener = l;
		return true;
	}
	int   Parse(unsigned char *buffer, int data_len);


private:
	I_nalu_parser   *m_listener;
	bool           m_find1;
	int            m_count;

	int find_start_code(const unsigned char *buffer, int data_len);
	void process_data(unsigned char *buffer, int data_len);
	int  parse_packet(unsigned char *buffer, int data_len);
};

#endif  // __NALU_PARSER_H__