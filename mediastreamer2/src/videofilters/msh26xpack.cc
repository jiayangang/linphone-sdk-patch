/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "h264pack.h"

class H26xPackState
{
public:
	h264_pack    _pack;
	void slice(mblk_t *im, MSQueue *rtpq, uint32_t ts)
	{
		_pack.feed(im, rtpq, ts);
	}
};

static void h26x_pack_process(MSFilter *f){
	H26xPackState *s=(H26xPackState*)f->data;
	mblk_t *im;
	uint32_t timestamp;

	ms_filter_lock(f);
	
	while((im=ms_queue_get(f->inputs[0]))!=NULL )
	{
		//ms_queue_put(f->outputs[0], im);
		//*
		timestamp = mblk_get_timestamp_info(im);
		s->slice(im, f->outputs[0], timestamp);
		freemsg(im);
		//*/
	}
	ms_filter_unlock(f);
}

static void h26x_pack_postprocess(MSFilter *f){
	//H26xPackState *s=(H26xPackState*)f->data;
}

static void h26x_pack_preprocess(MSFilter *f)
{
	H26xPackState *s = (H26xPackState*)f->data;
	s->_pack.initialize();
}

static void h26x_pack_uninit(MSFilter *f){
	H26xPackState *s=(H26xPackState*)f->data;
	delete s;
}

static void h26x_pack_init(MSFilter *f){
	H26xPackState *s= new H26xPackState();
	f->data=s;
}

/*
static int h26x_pack_set_vsize(MSFilter *f, void*arg){
	H26xPackState *s=(H26xPackState*)f->data;
	ms_filter_lock(f);
	ms_filter_unlock(f);
	return 0;
}

static int h26x_pack_set_fps(MSFilter *f, void *arg){
	H26xPackState *s=(H26xPackState*)f->data;
	return 0;
}


static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	h26x_pack_set_fps	},
	{	MS_FILTER_SET_VIDEO_SIZE, h26x_pack_set_vsize	},
	{	0	,	NULL }
};
*/
#ifdef __cplusplus
extern "C" {
#endif


#ifdef _MSC_VER

MSFilterDesc ms_h26x_pack_desc={
	MS_H26X_PACK_ID,
	"MSH26xPack",
	"A video size converter", //N_("A video size converter"),
	MS_FILTER_OTHER,
	NULL,
	1,
	1,
	h26x_pack_init,
	h26x_pack_preprocess,
	h26x_pack_process,
	h26x_pack_postprocess,
	h26x_pack_uninit,
	NULL  //methods
};

#else

MSFilterDesc ms_h26x_pack_desc={
	.id=MS_H26X_PACK_ID,
	.name="MSH26xPack",
	.text=N_("a small video size converter"),
	.ninputs=1,
	.noutputs=1,
	.init=h26x_pack_init,
	.preprocess = h26x_pack_preprocess,
	.process=h26x_pack_process,
	.postprocess=h26x_pack_postprocess,
	.uninit=h26x_pack_uninit
};

#endif

#ifdef __cplusplus
}
#endif