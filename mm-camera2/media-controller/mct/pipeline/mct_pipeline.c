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
	pipeline->process_serv_msg= mct_pipeline_process_serv_msg;
	pipeline->process_bus_msg = mct_pipeline_process_bus_msg;
	....

	return pipeline;
}
