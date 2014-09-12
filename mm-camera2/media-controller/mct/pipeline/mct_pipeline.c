static cam_dimention_t default_liveshot_sizes[] = {
	{ 4128, 3096}, // 4:3
	{ 4128, 2322}, // 16:9
	{ 4000, 3000}, // 12MP
	{ 3200, 2400}, // 8MP
	{ 2592, 1944}, // 5MP
	{ 2048, 1536}, // 3MP QXGA
	{ 1920, 1080}, // HD1080
	{ 1600, 1200}, // 2MP UXGA
	{ 1280, 768},  // WXGA
	{ 1280, 720},  // HD720
	{ 1024, 768},  // 1MP XGA
	{ 800, 600},   // SVGA
	{ 864, 480},   // FWVGA
	{ 800, 480},   // WVGA
	{ 720, 480},   // 480p
	{ 640, 480},   // VGA
	{ 352, 288},   // CIF
	{ 320, 240},   // QVGA
	{ 176, 144}    // QCIF
};

/** mct_pipeline_send_event
 *	@pipeline
 *	@stream_id
 *  @event
 * 
 *	Return: TRUE on success, FALSE on failure
 **/
static boolean mct_pipeline_send_event(mct_pipeline_t *pipeline,
	unsigned int stream_id, mct_event_t *event)
{
	boolean ret = TRUE;
	mct_stream_t *stream = NULL;
	mct_pipeline_get_stream_info_t info;
	
	if(!pipeline || !event)
		return FALSE;
		
	info.check_type = CHECK_INDEX;
	info.stream_index = stream_id;
	
	stream = mct_pipeline_get_stream(pipeline, &info);
	if(!stream){
		CDBG_ERROR("%s: Couldn't find stream\n", __func__);
		return FALSE;
	}
	
	ret = stream->send_event(stream, event);
	return ret;
}

/**	mct_pipeline_send_ctrl_events:
 *
 *
 *
 **/
boolean mct_pipeline_send_ctrl_events(mct_pipeline_t *pipeline,
	unsigned int stream_id, mct_event_control_type_t event_type)
{
	....
	if(pipeline->send_event){
		ret = pipeline->send_event(pipeline, stream_id, &cmd_event);
		if(ret == FALSE)
			break;
	}else{
		break;
	}
	
	....
}

/** populate_query_cap_buffer:
 *		@
 *		@
 *
 **/
static boolean mct_pipeline_populate_query_cap_buffer(mct_pipeline_t *pipeline)
{
	....
	hal_data->livesnapshot_sizes_tbl_cnt = 0;
	for(i = 0;
		(i < (sizeof(default_liveshot_sizes) / sizeof(cam_dimension_t))
		&& i < MAX_SIZES_CNT);
		i++){
		if(default_liveshot_sizes[i].width <= liveshot_sensor_width &&
		   default_liveshot_sizes[i].height <= liveshot_sensor_height){
		   hal_data->livesnapshot_sizes_tbl[hal_data->livesnapshot_sizes_tbl_cnt] =
		   		default_liveshot_sizes[i];
		   hal_data->livesnapshot_sizes_tbl_cnt++;
		   }
		}
	....
}

/** mct_pipeline_process_set:
 * @
 * @
 * 
 * */
static boolean mct_pipeline_process_set(struct msm_v4l2_event_data *data,
	mct_pipeline_t *pipeline)
{
	boolean ret = TRUE;
	mct_stream_t *stream = NULL;
	mct_pipeline_get_stream_info_t info;
	
	ALOGE("%s: command = %x", __func__, data->command);
	
	/* First find correct stream; for some commands find based on index,
	 * for others (session based commands) find the appropriate stream
	 * based on stream_type */
	switch(data->command){
	....
	
	case CAM_PRIV_PRAM:{
		/*This case could potentially hit even before a stream exists */
		if(MCT_PIPELINE_NUM_CHILDREN(pipeline) > 0){
			info.check_type		= CHECK_TYPE;
			info.stream_type	= CAM_STREAM_TYPE_PREVIEW;
			stream = mct_pipeline_get_stream(pipeline, &info);
			if(!stream){
				info.check_type = CHECK_TYPE;
				info.stream_type = CAM_STREAM_TYPE_RAW; /*RDI streaming*/
				stream = mct_pipeline_get_stream(pipeline, &info);
				if(!stream){
					CDBG_ERROR("%s: Couldn't find preview stream; Storing for later\n", __func__);
				}
			}
		}
	}
	break;
	
	default:
	break;
	}
	
	/* Now process the set ctrl command on the appropriate stream */
	switch(data->command){
	....
	case CAM_PRIV_PARM:{
		/*start reading the parm buf from HAL*/
		parm_buffer_new_t *p_table = (parm_buffer_new_t *)pipeline->config_parm;
		
		if(p_table && p_table->num_entry){
			CDBG("%s: num params in set_parm: %d", __func__, p_table->num_entry);
			copy_pending_parm(p_table, pipeline->pending_set_parm);
			sem_post(&p_table->cam_sync_sem);
		}
		if(stream){
			/*send event only if a stream exists*/
			ret = mct_pipeline_send_ctrl_events(pipeline, stream->streamid,
				MCT_EVENT_CONTROL_SET_PARM);
		}else{
			ret = TRUE;
		}
	}
	....
	}
	
}

/** mct_pipeline_process_get:
 *	@
 *	@
 *
 **/
static boolean mct_pipeline_process_get(struct msm_v4l2_event_data *data, mct_pipeline_t *pipeline)
{
	boolean ret = TRUE;
	mct_stream_t *stream = NULL;
	mct_pipeline_get_stream_info_t info;
	
	....
	
	switch(data->command){
	case MSM_CAMERA_PRIV_QUERY_CAP:{
		/* for queryBuf */
		/* czm command case from camera.c(kernel/media/platform/msm/camera_v2/camera/)*/
		if(!pipeline->query_buf || !pipeline->modules){
			ret = FALSE;
		}else{
			memset(&pipeline->query_data, 0, sizeof(mct_pipeline_cap_t));
			mct_list_traverse(pipeline->modules, mct_pipeline_query_modules,
			pipeline);
		}
		/*now fill up HAL's query buffer*/
		ret = mct_pipeline_populate_query_cap_buffer(pipeline);
	}
	break;
	....
	}
	return ret;
}

/** mct_pipeline_process_serv_msg:
 *	@
 *	@
 *
 **/
static boolean mct_pipeline_process_serv_msg(void *message,
 mct_pipeline_t *pipeline)
{
	struct v4l2_event *msg = (struct v4l2_event *)message;
	mct_stream_t *stream = NULL;
	boolean ret = TRUE;
	struct msm_v4l2_event_data *data = 
	(struct msm_v4l2_event_data *)(msg->u.data);
	
	if(!message || !pipeline || data->session_id != pipeline->session)
		return FALSE;
	
	switch(msg->id){
	case MSM_CAMERA_SET_PARM:
		ret = mct_pipeline_process_set(data, pipeline);
		break;
		
	case MSM_CAMERA_GET_PARM:
		/* process config_w */
		ret = mct_pipeline_process_get(data, pipeline);
		break;
	
	....
	}
	
	return ret;
}

/** mct_pipeline_new
 *
 **/
mct_pipeline_t* mct_pipeline_new(void)
{
	mct_pipeline_t *pipeline;
	
	pipeline = malloc(sizeof(mct_pipeline_t));
	if (!pipeline)
		return NULL;
	memset(pipeline, 0, sizeof(mct_pipeline_t));
	
	....
	
	/* For case SERV_MSG_SET,SERV_MSG_GET, SERV_MSG_STREAMON, SERV_MSG_STREAMOFF,
	SERV_MSG_QUERY,SERV_MSG_CLOSE_SESSION */
	pipeline->process_serv_msg = mct_pipeline_process_serv_msg;
	pipeline->process_bus_msg = mct_pipeline_process_bus_msg;
	....

	pipeline->send_event = mct_pipeline_send_event;
	....
	return pipeline;
}
