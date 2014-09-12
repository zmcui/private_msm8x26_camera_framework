/** mct_stream_start_link:
 *	@stream: stream object of mct_stream_t
 *
 *	Analysis stream information and links corresponding
 *	modules for this stream
 *
 *	Return FASLE if the stream is not able to be linked.
 **/

static boolean mct_stream_start_link(mct_stream_t *stream)
{
	int sessionid;
	cam_stream_info_t	*stream_info;
	boolean				ret = FALSE;
	mct_stream_map_buf_t *info = MCT_STREAM_STREAMINFO(stream);
	mct_list_t			*modules =
		MCT_PIPELINE_MODULES(MCT_OBJECT_PARENT(stream)->data);
	mct_pipeline_t		*pipeline =
		MCT_PIPELINE_CAST((MCT_STREAM_PARENT(stream))->data);
		
	mct_module_t *sensor	= NULL;
	mct_module_t *iface		= NULL;
	mct_module_t *isp		= NULL;
	mct_module_t *stats		= NULL;
	mct_module_t *pproc		= NULL;
	mct_module_t *imglib	= NULL;
	mct_module_t *hdr		= NULL;
	
	if(info == NULL)
		return FALSE;
		
	stream_info = (cam_stream_info *)info;
		
	sessionid = MCT_PIPELINE_SESSION(
		MCT_PIPELINE_CAST(MCT_STREAM_PARENT(stream))->data));
		
	stream->streaminfo.identity = pack_identity(sessionid, stream->streamid);
	stream->streaminfo.stream_type = stream_info->stream_type;
	stream->streaminfo.fmt = stream_info->fmt;
	stream->streaminfo.dim = stream_info->dim;
	stream->streaminfo.streaming_mode = stream_info->streaming_mode;
	stream->streaminfo.num_burst = stream_info->num_of_burst;
	stream->streaminfo.buf_planes = stream_info->buf_planes;
	stream->streaminfo.pp_config = stream_info->pp_config;
	stream->streaminfo.reprocess_config = stream_info->reprocess_config;
	stream->streaminfo.num_bufs = stream_info->num_bufs;
	stream->streaminfo.is_type = stream_info->is_type;
	/* TODO: temporary solution for now */
	stream->streaminfo.stream = stream;
	
	mct_pipeline_add_stream_to_linked_streams(pipeline, stream);
	
	switch(stream->streaminfo.stream_type){
	case CAM_STREAM_TYPE_POSTVIEW:{
	....
	}
	  break;
	
	case CAM_STREAM_TYPE_PREVIEW:{
		
		CDBG_HIGH("%s: Starting preview/postview stream linking \n", __func__);
		
		sensor = mct_stream_get_module(modules, "sensor");
		iface = mct_stream_get_module(modules, "iface");
		isp = mct_stream_get_module(modules, "isp");
		stats = mct_stream_get_module(modules, "stats");
		pproc = mct_stream_get_module(modules, "pproc");
		imglib = mct_stream_get_module(modules, "imglib");
		
		if(!sensor || !iface || !isp ||!stats || !pproc || !imglib){
			ALOGE("%s:%s] Null: %p %p %p %p %p %p", __func__, __LINE__,
				sensor, iface, isp, stats, pproc, imglib);
			return FALSE;
		}
		sensor->set_mod(sensor, MCT_MODULE_FLAG_SOURCE,
			stream->streaminfo.identity
		iface->set_mod(iface, MCT_MODULE_FLAG_INDEXABLE,
			stream->streaminfo.identity);
		isp->set_mod(isp, MCT_MODULE_FLAG_INDEXABLE,
			stream->streaminfo.identity);
		stats->set_mod(stats, MCT_MODULE_FLAG_SINK,
			stream->streaminfo.identity);
		pproc->set_mod(pproc, MCT_MODULE_FLAG_INDEXABLE,
			stream->streaminfo.identity);
		imglib->set_mod(imglib, MCT_MODULE_FLAG_SINK,
			stream->streaminfo.identity);
		
		ret = mct_stream_link_modules(stream, sensor, iface, isp, pproc, imglib, 
			NULL);
		if(ret == FALSE){
			CDBG_ERROR("%s:%d] link failed", __func__, __LINE__);
			return FALSE;
		}
		if(pipeline->query_data.sensor_cap.sensor_format != FORMAT_YCBCR){
			ret = mct_stream_link_modules(stream, isp, stats, NULL);
			if(ret = FALSE){
			CDBG_ERROR("%s:%d] link failed", __func__, __LINE__);
			return FALSE;
			}
		}
		
		if(ret == FALSE){
			CDBG_ERROR("%s:%d] link failed", __func__, __LINE__);
			return FALSE;
		}
		ret = mct_pipeline_send_ctrl_events(pipeline, stream->streamid,
			MCT_EVENT_CONTROL_SET_PARM);
	}
		break;
		
	case CAM_STREAM_TYPE_SNAPSHOT: {
		....
	}
	....
	}/* switch (stream->streaminfo.stream_type)*/
	
	return TRUE;
}

/**
 *
 * Arguments/Fields:
 * 	@
 * 	@
 * 
 * 	Return
 * 
 * 	Description:
 * 
 */
static boolean mct_stream_send_event(mct_stream_t *stream, mct_event_t *event)
{
	mct_module_t *src_module = NULL;
	boolean ret = FALSE;
	
	if(stream->streaminfo.stream_type == CAM_STREAM_TYPE_METADATA){
		ret = mct_stream_metadata_ctrl_event(stream, event);
	}else{
		if(MCT_STREAM_CHILDREN(stream)){
			src_module = (mct_module_t *)(MCT_STREAM_CHILDREN(stream)->data);
		}
		if(src_module){
			if((mct_module_find_type(src_module, event->identity)
				== MCT_MODULE_FLAG_SOURCE) &&
				src_module->process_event){
			ret = src_module->process_event(src_module, event);
			}
		}
	}
	return ret;
}

/**
 *
 * Arguments/Fields:
 * 	@
 * 	@
 * 
 * 	Return
 * 
 * 	Description:
 * 
 */
mct_stream_t* mct_stream_new(unsigned int stream_id)
{
	....
	stream->add_module		= mct_stream_add_module;
	stream->remove_module	= mct_stream_remove_module;
	stream->sendevent		= mct_stream_send_event;
	stream->map_buf			= mct_stream_map_buf;
	stream->unmap_buf		= mct_stream_unmap_buf;
	stream->link			= mct_stream_start_link;
	stream->unlink			= mct_stream_start_unlink;
	
	return stream;
}
