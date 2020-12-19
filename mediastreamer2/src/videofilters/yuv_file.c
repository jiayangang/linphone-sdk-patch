#include "yuv_file.h"

int  yuv_file_write(yuv_write *s, uint8_t *ptr, int size)
{
    fwrite(ptr, 1, size, s->m_hFile);
    return 1;
}
int yuv_file_write_data(yuv_write *s, AVFrame *frame, int width, int height)
{
    int buf_size = height * width * 3 / 2;
    unsigned char* buf = (unsigned char*)malloc(buf_size);
    if(!buf)
        return 0;

    //memset(buf, 0, buf_size);
   
   
    int a = 0, i;
    for (i = 0; i<height; i++)
    {
        memcpy(buf + a, frame->data[0] + i * frame->linesize[0], width);
        a += width;
    }
    for (i = 0; i<height / 2; i++)
    {
        memcpy(buf + a, frame->data[1] + i * frame->linesize[1], width / 2);
        a += width / 2;
    }
    for (i = 0; i<height / 2; i++)
    {
        memcpy(buf + a, frame->data[2] + i * frame->linesize[2], width / 2);
        a += width / 2;
    }
		
	fwrite(buf, 1, buf_size, s->m_hFile);
    free(buf);
    return 1;
}

void yuv_file_close(yuv_write *s)
{
    if(s->m_hFile) fclose(s->m_hFile);
    s->m_hFile = NULL;
}

int yuv_file_open(yuv_write *s, const char *filepath)
{
	if(!filepath || *filepath == '\0')
        return 0;

    FILE *fh;
    if((fh = fopen(filepath, "wb")) == 0)
        return 0;

    s->m_hFile = fh;
    return 1;
}
