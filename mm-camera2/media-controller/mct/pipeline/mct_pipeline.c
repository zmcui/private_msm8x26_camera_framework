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

static cam_fps_range_t default_fps_ranges[] = {
  { 15.0, 15.0, 15.0, 15.0},
  { 24.0, 24.0, 24.0, 24.0},
  { 30.0, 30.0, 30.0, 30.0},
}

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

  cam_fps_range_t fps = {10.0, 30.0, 10.0, 30.0};

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
  //fps range
  hal_data->fps_ranges_tbl_cnt = 0;
  for(i = 0; 
     (i < (sizeof(default_fps_ranges) / sizeof(cam_fps_range_t))
       && i < MAX_SIZES_CNT);
     i++){
    if (default_fps_ranges[i].min_fps >= fps.min_fps &&
        default_fps_ranges[i].max_fps <= fps.max_fps) {
      hal_data->fps_ranges_tbl[hal_data->fps_rangs_tbl_cnt] =
       default_fps_ranges[i];
      hal_data->fps_ranges_tbl_cnt++;
    }
  }
  if (hal_data->fps_ranges_tbl_cnt == 0) {
    hal_data->fps_ranges_tbl[hal_data->fps_ranges_tbl_cnt] = fps;
    hal_data->fps_ranges_tbl_cnt++;
  }else{
    //fps range of sensor will replace the first entry that meets this condition
    for (i = 0; i < hal_data->fps_ranges_tbl_cnt && i < MAX_SIZES_CNT; i++){
      if ((hal_data->fps_ranges_tbl[i].max_fps > fps.max_fps) ||
        (F_EQAL(hal_data->fps_ranges_tbl[i].max_fps, fps.max_fps)) &&
        (hal_data->fps_ranges_tbl[i].min_fps > fps.min_fps)){
        break;
      }
    }
    if (MAX_SIZES_CNT > i) {
      for (j = hal_data->fps_ranges_tbl_cnt; j > i; j--) {
        hal_data->fps_ranges_tbl[j] = hal_data->fps_ranges_tbl[j-1];
      }
      hal_data->fps_ranges_tbl[i] = fps;
      hal_data->fps_ranges_tbl_cnt++;
    }
  }
  ....
  //effects
  hal_data->supported_effects_cnt = 0;
  if(local_data->sensor_cap.sensor_format == FORMAT_BAYER) {
    //depend on ISP cap
    for (i = 0; i < local_data->isp_cap.supported_effects_cnt; i++) {
      hal_data->supported_effects[hal_data->supported_effects_cnt] =
        local_data->isp_cap.supported_effects[i];
      hal_data->supported_effects_cnt++;
    }
  } else {
    for (i = 0; i < CAM_EFFECT_MODE_MAX; i++) {
    //depend on Sensor cap
      if (local_data->sensor_cap.sensor_supported_effect_modes & (1 << i)) {
        hal_data->supported_effects[hal_data->supported_effects_cnt] = i;
        hal_data->supported_effects_cnt++;
      }
    }
  }

  uint8_t supported_effects_cnt = hal_data->supported_effects_cnt;
    //depend on PP cap
  for (j = 0; j < local_data->pp_cap.supported_effects_cnt; j++){
    common = FALSE;
    //if sensor_cap and pp cap_overlap
    for (i = 0; i < supported_effects_cnt i++) {
      if (hal_data->supported_effects[i] ==
        local_data->pp_cap.supported_effects[j]) {
        common = TRUE;
        break;
      }
    }
    if (common == TRUE)
      continue;
    hal_data->supported_effects[hal_data->supported_effects_cnt] =
      local_data->pp_cap.supported_effects[j];
    hal_data->supported_effects_cnt++;
  }

  
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

/** mct_pipeline_start_session_thread:
 *    @data: void* pointer to the mct_pipeline_thread_data
 *
 *  Thread implementation to start camera module.
 *
 *  Return: NULL
 **/
static void* mct_pipeline_start_session_thread(void *data)
{
  mct_pipeline_thread_data_t *thread_data = (mct_pipeline_thread_data_t *)data;
  mct_module_t *module = thread_data->module;
  unsigned int session_id = thread_data->session_id;
  CDBG_ERROR("%s thread_id is %d\n", __func__, syscall(SYS_gettid));
  pthread_mutex_lock(&thread_data->mutex);
  pthread_cond_signal(&thread_data->cond_v);
  pthread_mutex_unlock(&thread_data->mutex);

  if (module->start_session){
    module->start_session(module, session_id);
  }

  pthread_mutex_lock(&thread_data->mutex);
  thread_data->started_num++;
  if(thread_data->started_num == thread_data->modules_num)
    pthread_cond_signal(&thread_data->cond_v);
  pthread_mutex_unlock(&thread_data->mutex);

  return NULL;
}

/** mct_pipeline_modules_start:
 *    @data1: void* pointer to the module being processed
 *    @data2: void* pointer to the pipeline object
 *
 *  Create thread for each module for start session.
 *
 *  Return: TRUE on success
 **/
static boolean mct_pipeline_modules_start(void *data1, void *data2)
{
  int rc = 0;
  pthread_attr_t attr;
  mct_pipeline_t *pipeline = (mct_pipeline_t *)data2;
  mct_pipeline_thread_data_t *thread_data = &(pipeline->thread_data);
  thread_data->module = (mct_module_t *)data1;
  thread_data->session_id = pipeline->session;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_lock(&thread_data->mutex);
  rc = pthread_create(&pipeline->thread_data.pid, &attr,
      &mct_pipeline_start_session_thread, (void *)thread_data);
  if(!rc){
    pthread_cond_wait(&thread_data->cond_v, &thread_data->mutex);
  }
  pthread_mutex_unlock(&thread_data->mutex);

  return TRUE;
}

/** mct_pipeline_start_session:
 *    @pipeline: mct_pipeline_t object
 *
 *  Pipeline start session, start each camera module and query the
 *  module capacity
 *
 *  Return: no return value
 **/
void mct_pipeline_start_session(mct_pipeline_t *pipeline)
{
  //czm mutex and condition usage
  pthread_mutex_init(&pipeline->thread_data.mutex, NULL);
  pthread_cond_init(&pipeline->thread_data.cond_v, NULL);
  pipeline->thread_data.started_num = 0;
  pipeline->thread_data.modules_num = 0;
  mct_list_traverse(pipeline->modules, mct_pipeline_get_module_num,
      pipeline);
  //czm start each module
  mct_list_traverse(pipeline->modules, mct_pipeline_modules_start,
      pipeline);
  pthread_mutex_lock(&pipeline->thread_data.mutex);
  pthread_cond_wait(&pipeline->thread_data.cond_v, &pipeline->thread_data.mutex);
  pthread_mutex_unlock(&pipeline->thread_data.mutex);
  mct_list_traverse(pipeline->modules, mct_pipeline_query_modules,
      pipeline);

  pipeline->max_pipeline_frame_delay = 2;

  //start tuning server
  mct_start_tuning_server(pipeline);

  return;
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
