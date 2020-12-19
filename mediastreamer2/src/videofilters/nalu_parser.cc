/*****************************************************************************

				��Ȩ���� (C), 2001-2050,

******************************************************************************
	�ļ����� : nalu_parser.cpp
	����     : ���Ӹ� 
	�������� : 2020-10-29 

	�汾     : 1.0
	�������� : 
	           

	˵��     ��


******************************************************************************/
#include "nalu_parser.h"

nalu_parser::nalu_parser(void)
{
	m_listener = NULL;
	m_find1 = false;
	m_count = 0;
}

/*
���NALUͷ
���������
00 00 01
00 00 00 01

����ֵ��
δ�ҵ������� 0
�ҵ�������ͷ�ĳ��� 3 ���� 4
*/
int nalu_parser::find_start_code(const unsigned char *buffer, int data_len)
{
	if (!buffer || data_len <= 0)
		return 0;

	// ��� 00
	const unsigned char *p = buffer;
	if (*p != 0x00)
		return 0;

	p++;
	data_len--;
	if (data_len < 1) return 0;
	if (*p != 0x00)
		return 0;

	p++;
	data_len--;
	if (data_len < 1) return 0;
	if (*p == 0x01)   // 00 00 01
		return 3;

	// ����3�ֽ��ǲ��� 00
	if (*p != 0x00)
		return 0;

	p++;
	data_len--;
	if (data_len < 1) return 0;
	return (*p == 0x01) ? 4 : 0;
}

int nalu_parser::parse_packet(unsigned char *buffer, int data_len)
{
	int result = 0;
	unsigned char *p = buffer;
	unsigned char *p_start = p;
	if (!m_find1)
	{
		while (data_len > 0)
		{
			int start_code_len = find_start_code(p, data_len);
			if (0 == start_code_len)
			{
				p++;
				data_len--;
				continue;
			}

			m_find1 = true;
			data_len -= start_code_len;
			p += start_code_len;
			p_start = p;
			break;
		}
	}

	if (!m_find1)
		return data_len;


	// ���ҵڶ���
	bool find2 = false;
	int start_code_len = 0;
	while (data_len > 0)
	{
		start_code_len = find_start_code(p, data_len);
		if (0 == start_code_len)
		{
			p++;
			data_len--;
			continue;
		}

		find2 = true;
		break;
	}

	if (find2)
	{
		int len = p - p_start;
		process_data(p_start, len);

		data_len -= start_code_len;
		p += start_code_len;
		result = p - buffer;
	}
	return result;
}

int nalu_parser::Parse(unsigned char *buffer, int data_len)
{
	unsigned char *p = buffer;
	while (1)
	{
		int len = parse_packet(p, data_len);
		if (len <= 0)
			break;

		//printf("consume %d\n", len);
		p += len;
		data_len -= len;

	}
	if (data_len > 0) process_data(p, data_len);
	return 0;
}

void nalu_parser::process_data(unsigned char *buffer, int data_len)
{
	if (!buffer || data_len <= 0)
		return;

	int type = *buffer & 0x1F;
	printf("%d  type %d :len = %d\n", m_count++, type, data_len);
	if (m_listener) m_listener->cbNaluPacket(type, buffer, data_len);
}