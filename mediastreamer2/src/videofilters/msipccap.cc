/*****************************************************************************

					��Ȩ���� (C), 2001-2050,

******************************************************************************
	�ļ����� : h264pack.cc
	����     : ���Ӹ�
	�������� : 2020-11-04

	�汾     : 1.0
	�������� :
				IPC����ͷ����ȡRTSP��

	˵��     ��
	

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/mswebcam.h"
#include <math.h>
#include "mslivestream.h"
#include "h264pack.h"

class LiveYuv420P
{
public:
	LiveYuv420P() 
	{
		qinit(&_rq);
		ms_mutex_init(&_mutex, NULL);
		_vsize.width = 384;// MS_VIDEO_SIZE_CIF_W;
		_vsize.height = MS_VIDEO_SIZE_CIF_H;
		_fps = 25;
		_pixfmt = MS_YUV420P;
		_ready = false;
		_url = NULL;

		_strm_opened = false;
		_has_decoded = false;
		m_camStream = NULL;
	}
	virtual ~LiveYuv420P() 
	{
		if (_ready) stopAndClean();
		flushq(&_rq, 0);
		if (_url) {
			ms_free(_url);
			_url = NULL;
		}
		ms_mutex_destroy(&_mutex);
	}

public:
	mblk_t *readFrame();
		

	void slice(mblk_t *im, MSQueue *rtpq, uint32_t ts);
	bool OpenLive2();

	bool OpenLive();
	void CloseLive();
	static void cbLive_StreamData(int dataType, YuvBuf *yuv420p, void* userData);
	void OnLive_StreamData(int dataType, YuvBuf *yuv420p);

	bool_t isTimeToSend(uint64_t ticker_time);
	MSVideoSize getVSize() {
		if (!_ready) createDshowGraph(); /* so that _vsize is updated according to hardware capabilities*/

		//_vsize.width = MS_VIDEO_SIZE_CIF_W;
		//_vsize.height = MS_VIDEO_SIZE_CIF_H;
		return _vsize;
	}
	void setVSize(MSVideoSize vsize) {
		_vsize = vsize;
	}
	void setFps(float fps) {
		_fps = fps;
	}
	float getFps() {
		return _fps;
	}
	MSPixFmt getPixFmt() {
		if (!_ready) createDshowGraph(); /* so that _pixfmt is updated*/
		return _pixfmt;
	}
	int GetSourceFormat();
	void setDeviceIndex(const char *url) {
		if (_url) {
			ms_free(_url);
			_url = NULL;
		}
		if (url && *url != '\0') _url = ms_strdup(url);
	}
	MSAverageFPS avgfps;
	MSFrameRateController framerate_controller;
	bool         _has_decoded;

private:
	char        *_url;
	MSVideoSize  _vsize;
	queue_t      _rq;
	ms_mutex_t   _mutex;
	float        _fps;
	MSPixFmt     _pixfmt;
	bool         _ready;
	bool         _strm_opened;
	
	CamStream   *m_camStream;
	h264_pack    _pack;
	
	int createDshowGraph();
	void stopAndClean();
};

void LiveYuv420P::stopAndClean() {
	
	ms_mutex_lock(&_mutex);
	flushq(&_rq, 0);
	ms_mutex_unlock(&_mutex);
	_ready = false;
}
int LiveYuv420P::createDshowGraph() 
{
	_ready = true;
	return 0;
}
bool_t LiveYuv420P::isTimeToSend(uint64_t ticker_time) {
	return ms_video_capture_new_frame(&framerate_controller, ticker_time);
}

void LiveYuv420P::cbLive_StreamData(int dataType, YuvBuf *yuv420p, void* userData)
{
	LiveYuv420P *d = (LiveYuv420P*)userData;
	d->OnLive_StreamData(dataType, yuv420p);
}
void LiveYuv420P::OnLive_StreamData(int dataType, YuvBuf *yuv420p)
{
	mblk_t *mb = NULL;
	if (CAM_LIVE_TYPE_H264 == dataType) {  // H264 or H265
		mb = ms_buf_copy_from_buf(yuv420p->planes[0], yuv420p->strides[0], yuv420p->w, yuv420p->h);
	}
	else {  
		// YUV420P
		mb = ms_buf_copy_from_ms_frame(yuv420p, yuv420p->w, yuv420p->h);
	}
	ms_mutex_lock(&_mutex);
	if(mb) putq(&_rq, mb);
	ms_mutex_unlock(&_mutex);
}

mblk_t* LiveYuv420P::readFrame()
{
	mblk_t *ret = NULL;
	ms_mutex_lock(&_mutex);
	ret = getq(&_rq);
	ms_mutex_unlock(&_mutex);
	return ret;
}

void LiveYuv420P::CloseLive()
{
	if (m_camStream)
	{
		m_camStream->Close();
		delete m_camStream;
		m_camStream = NULL;
	}
}

bool LiveYuv420P::OpenLive2()
{
	m_camStream = create_CamStream(_url);
	if (!m_camStream)
		return false;

	return m_camStream->open(_url);
}


bool LiveYuv420P::OpenLive()
{
	if (!_strm_opened)
	{
		_strm_opened = OpenLive2();
	}
	if (_strm_opened)
	{
		unsigned int fmts = m_camStream->support_fmts();
		if (!(fmts & CAM_LIVE_TYPE_H264))
			_has_decoded = true;

		m_camStream->read_stream(cbLive_StreamData, this, _has_decoded);
		_pack.initialize();	
	}
	return _strm_opened;
}

int LiveYuv420P::GetSourceFormat()
{  
	//return MS_SOURCE_FMT_ENCODED;

	if (!_strm_opened)
	{
		_strm_opened = OpenLive2();
	}
	if (_strm_opened)
	{
		unsigned int fmts = m_camStream->support_fmts();
		if (fmts & CAM_LIVE_TYPE_H264)  // H264�򲻽���
		{
			//_strm_opened = false;
			return MS_SOURCE_FMT_ENCODED;
		}
	}
	
	//_strm_opened = false;
	return MS_SOURCE_FMT_DEFAULT;
}

void LiveYuv420P::slice(mblk_t *im, MSQueue *rtpq, uint32_t ts)
{
	_pack.feed(im, rtpq, ts);
}

/*
#include "../otherfilters/str_t.h"
#include "../otherfilters/filerw_t.h"

static int s_temp = 1;
void write_one_file(const unsigned char *buffer, size_t data_len, uint32_t ts)
{
	str_t str;
	str.format("c:\\aaa-rtsp\\pkg_%d_%d.264", s_temp++, ts);

	filerw_t rwt;
	rwt.open(str.c_str(), F_TYPE_WRITE);
	rwt.write(buffer, data_len);
	rwt.close();
}
*/
static void msipccap_process(MSFilter *obj) {
	LiveYuv420P *s = (LiveYuv420P*)obj->data;
	uint32_t timestamp;

    //ms_error("-----------1111----------------%d", (uint32_t)obj->ticker->time);
	//if (s->isTimeToSend(obj->ticker->time)) 
	{
		mblk_t *om = s->readFrame();
		if (om != NULL) 
		{
			timestamp = (uint32_t)(obj->ticker->time * 90);/* rtp uses a 90000 Hz clockrate for video*/

			if (s->_has_decoded)
			{
				mblk_set_timestamp_info(om, timestamp);
				ms_queue_put(obj->outputs[0], om);
			}
			else {
				mblk_set_timestamp_info(om, timestamp);
				mblk_set_marker_info(om, 1);
				ms_queue_put(obj->outputs[0], om);

				//write_one_file(om->b_rptr, (om->b_wptr - om->b_rptr), timestamp);
				/*
				s->slice(om, obj->outputs[0], timestamp);
				freemsg(om);
				*/
				//Sleep(10);
			}
			

			ms_average_fps_update(&s->avgfps, obj->ticker->time);
		}
	}
}

void msipccap_postprocess(MSFilter *f) {
	LiveYuv420P *d = (LiveYuv420P*)f->data;
	d->CloseLive();
}
static void msipccap_preprocess(MSFilter *f)
{
	LiveYuv420P *d = (LiveYuv420P*)f->data;

	//ms_mutex_lock(&f->lock);
	d->OpenLive();
	ms_usleep(100 * 1000);
	//ms_mutex_unlock(&f->lock);	
}
void msipccap_uninit(MSFilter *f) {
	ms_free(f->data);
}

void msipccap_init(MSFilter *f){
	LiveYuv420P *s = new LiveYuv420P();
	f->data = s;
}


static int msipccap_set_vsize(MSFilter *f, void* arg){
	LiveYuv420P *s = (LiveYuv420P*)f->data;
	s->setVSize(*((MSVideoSize*)arg));
	return 0;
}
static int msipccap_get_fps(MSFilter *f, void *arg) {
	LiveYuv420P *s = (LiveYuv420P*)f->data;
	if (f->ticker) {
		*((float*)arg) = ms_average_fps_get(&s->avgfps);
	}
	else {
		*((float*)arg) = s->getFps();
	}
	return 0;
}
static int msipccap_set_fps(MSFilter *f, void* arg){
	LiveYuv420P *s = (LiveYuv420P*)f->data;
	s->setFps(*(float*)arg);
	ms_video_init_framerate_controller(&s->framerate_controller, *(float*)arg);
	ms_average_fps_init(&s->avgfps, "msdscap: fps=%f");
	return 0;
}

static int msipccap_get_fmt(MSFilter *f, void* data){
	*(MSPixFmt*)data=MS_YUV420P;
	return 0;
}

static int msipccap_get_vsize(MSFilter *f, void* arg){
	LiveYuv420P *s = (LiveYuv420P*)f->data;
	MSVideoSize *vs = (MSVideoSize*)arg;
	*vs = s->getVSize();
	return 0;
}

static int msipccap_get_source_fmt(MSFilter *f, void* arg) {
	LiveYuv420P *s = (LiveYuv420P*)f->data;
	int *value = (int*)arg;

	//ms_mutex_lock(&f->lock);
	*value = s->GetSourceFormat();
	//ms_mutex_unlock(&f->lock);

	//ms_error("-----------****** UI CUSTOM ******   msipccap_get_source_fmt = %d", (int)*value);
	
	return 0;
}

static int msipccap_set_cam_param(MSFilter *f, void* arg) {
	LiveYuv420P *s = (LiveYuv420P*)f->data;
	s->setDeviceIndex((char*)arg);
	return 0;
}

MSFilterMethod msipccap_methods[]={
	{	MS_FILTER_SET_VIDEO_SIZE, msipccap_set_vsize },
	{	MS_FILTER_SET_FPS,        msipccap_set_fps	},
	{   MS_FILTER_GET_FPS,        msipccap_get_fps },
	{	MS_FILTER_GET_PIX_FMT	, msipccap_get_fmt	},
	{	MS_FILTER_GET_VIDEO_SIZE, msipccap_get_vsize },
	{   MS_FILTER_GET_SOURCE_FMT, msipccap_get_source_fmt },
	{   MS_FILTER_SET_CAM_PARAM,  msipccap_set_cam_param },
	{	0,0 }
};

MSFilterDesc ms_liveyuv_desc={
	MS_MS_LIVE_ID,
	"MSLive",
	"A filter that outputs camera live stream",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	msipccap_init,
	msipccap_preprocess,
	msipccap_process,
	msipccap_postprocess,
	msipccap_uninit,
	msipccap_methods,
	0
};

MS_FILTER_DESC_EXPORT(ms_liveyuv_desc)

static void ms_liveyuv_detect(MSWebCamManager *obj);
static MSFilter *ms_liveyuv_create_reader(MSWebCam *obj)
{
	MSFilter *f = ms_factory_create_filter_from_desc(ms_web_cam_get_factory(obj), &ms_liveyuv_desc);
	return f;
}


static bool_t ms_liveyuv_encode_to_mime_type(MSWebCam *obj, const char *mime_type)
{
	if (0 == strcmp(mime_type, "H264") ||
		0 == strcmp(mime_type, "H265"))
		return TRUE;

	return FALSE;
}

#ifdef _MSC_VER
extern "C" {
#endif
MSWebCamDesc ms_liveyuv_camera_desc={
	"IPC Camera",
	&ms_liveyuv_detect,
	NULL,
	&ms_liveyuv_create_reader,
	NULL,
	NULL   //&ms_liveyuv_encode_to_mime_type
};
#ifdef _MSC_VER
}
#endif

MSWebCamDesc *ms_liveyuv_camera_desc_get(void){
	return &ms_liveyuv_camera_desc;
}

static void ms_liveyuv_detect(MSWebCamManager *obj){

	MSWebCam *cam=ms_web_cam_new(&ms_liveyuv_camera_desc);
	cam->name = ms_strdup("Live Stream(YUV)");
	cam->data = NULL;
	ms_web_cam_manager_add_cam(obj, cam);
}



/*
static void ms_liveyuv_copy(LiveYuv420P *d, MSFrame *picture)
{
MSPicture pict;
memcpy(d->pict.planes[0], picture->planes[0], picture->strides[0] * picture->h);
memcpy(d->pict.planes[1], picture->planes[1], picture->strides[1] * picture->h / 4);
memcpy(d->pict.planes[2], picture->planes[2], picture->strides[2] * picture->h / 4);
}
*/