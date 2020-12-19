#ifndef __YUV_FILE_H__
#define __YUV_FILE_H__

#include "ffmpeg-priv.h"

#ifdef __cplusplus
extern "C"{
#endif


typedef struct yuv_write{
    FILE             *m_hFile;
    long              m_file_size;
}yuv_write;


int  yuv_file_open(yuv_write *s, const char *filepath);
void yuv_file_close(yuv_write *s);
int  yuv_file_write_data(yuv_write *s, AVFrame *frame, int width, int height);
int  yuv_file_write(yuv_write *s, uint8_t *ptr, int size);

#ifdef __cplusplus
}
#endif

#endif // __YUV_FILE_H__
