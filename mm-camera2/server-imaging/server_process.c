static mct_module_init_name_t modules_list[] = {
  // goto modules/sensors/module/module_sensor.c
	{"sensor", module_sensor_init, module_sensor_deinit},
	{"iface", module_iface_init, module_iface_deinit},
  // goto modules/isp/module_isp.c
	{"isp", module_isp_init, module_isp_deinit},
	{"stats", stats_module_init, stats_module_deinit},
  // goto modules/pproc-new/pproc_module.c
	{"pproc", pproc_module_init, pproc_module_deinit},
	{"imglib", module_imglib_init, module_imglib_deinit},
};

/** server_process_module_init:
 *
 *  Very first thing Imaging Server performs after it starts
 *  to build up module list. One specific module initilization
 *  may not success because the module may not exist on this
 *  platform.
 **/
boolean server_process_module_init(void)
{
	mct_module_t *temp = NULL;
	int i;
	
	/* we could see this log at very first, e.g.
	01-21 07:50:23.850: E/mm-camera(286): server_process_module_init:59, int mods*/
	CDBG_ERROR("%s:%d, int mods", __func__, __LINE__);
	
	for(i = 0; i < (int)(sizeof(modules_list)/sizeof(mct_module_init_name_t)); i++){
		if(NULL == modules_list[i].init_mod)
			continue;
		
		temp = modules_list[i].init_mod(modules_list[i].name);
		if(temp){
			if((modules = mct_list_append(modules, temp, NULL, NULL)) == NULL){
				if(modules){
					for(i--; i >= 0; i--)
						modules_list[i].deinit_mod(temp);
					
					mct_list_free_all(modules, NULL);
					return FALSE;
				}
			}
		}
	}/*for*/
	
	return TRUE;
}

/** server_process_hal_event:
 * 	@event: v4l2_event from kernel
 * 
 * Process any command received from HAL through kernel
 * 
 * Return: process result, action server should take.
 **/
serv_proc_ret_t server_process_hal_event(struct v4l2_event *event)
{
	serv_proc_ret_t ret;
	mct_serv_msg_t serv_msg;
	struct msm_v4l2_event_data *data =
	(struct msm_v4l2_event_data *)(event->u.data);
	struct msm_v4l2_event_data *ret_data =
	(struct msm_v4l2_event_data *)(ret.ret_to_hal.ret_event.u.data);
	
	/* by default don't return command ACK to HAL,
	 * return ACK only for two cases:
	 * 1. new session
	 * 2. Failure
	 *
	 * other command will return after they are processed
	 * in MCT */
	ret.ret_to_hal.ret			= FALSE;
	ret.ret_to_hal.ret_type		= SERV_RET_TO_HAL_CMDACK;
	ret.ret_to_hal.ret_event	= *event;
	ret_data->v4l2_event_type	= event->type;
	ret_data->v4l2_event_id		= event->id;
	ret.result					= RESULT_SUCCESS;
	
	switch(event->id){
    //czm camera_pack_event(filep, MSM_CAMERA_NEW_SESSION, 0, -1, &event); from kernel
		case MSM_CAMERA_NEW_SESSION:{
      ret.ret_to_hal.ret = TRUE;
      ret.result = RESULT_NEW_SESSION;

      /*
       * new session starts, need to create a MCT:
       * open a pipe first.
       *
       * Note the 3 file descriptors:
       * one domain socket fd and two pipe fds are closed
       * at server side once session close information
       * is receivied by server.
       * */
      int pipe_fd[2];

      if (!pipe(pipe_fd)) {
        ret.new_session_info.mct_msg_rd_fd = pipe_fd[0];
        ret.new_session_info.mct_msg_wt_fd = pipe_fd[1];
      }else{
        goto error_return;
      }

      if(server_process_bind_hal_ds(data->session_id,
            &(ret.new_session_info.hal_ds_fd)) == FALSE){
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        goto error_return;
      }

      if (mct_controller_new(modules, data->session_id, pipe_fd[1]) == TRUE){
        ret.new_session = TRUE;
        ret.new_session_info.session_idx = data->session_id;
        goto process_done;
      }else{
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        goto error_return;
      }
    }/* case MSM_CAMERA_NEW_SESSION */
        break;

		case MSM_CAMERA_DEL_SESSION:
		....
		default:
		serv_msg.msg_type	= SERV_MSG_HAL;
		serv_msg.u.hal_msg	= *event;
		break;
	}/* switch (event->type) */
	
	// czm enter mct_controller.c(mm-camera2/media-controller/mct/controller/)
	if (mct_controller_proc_serv_msg(&serv_msg) == FALSE) {
		ret.result = RESULT_FAILURE;
		goto error_return;
	}
	
	....
}

/** server_process_hal_ds_packet:
 *	@fd
 *	@session
 *
 *	Return: serv_proc_ret_t
 *			FAILURE - will return to HAL immediately
 **/
serv_proc_ret_t server_process_hal_ds_packet(const int fd,
	const int session)
{
	....
	serv_msg.msg_type	= SERV_MSG_DS;
	serv_msg.u.hal_msg	= *event;
	....
	
	// czm enter mct_controller.c(mm-camera2/media-controller/mct/controller/)
	if (mct_controller_proc_serv_msg(&serv_msg) == FALSE) {
		ret.result = RESULT_FAILURE;
		goto error_return;
	}
	....
}

/*
 * server_process_mct_msg:
 *  @fd: pipe fd media controller uses to send message to HAL
 *  @session: session index
 *
 * Return: serv_proc_index
 *         FAILURE - will return to HAL immediately
 *
 * */
serv_proc_ret_t server_process_mct_msg(const int fd, const unsigned int session)
{
  int read_len;
  mct_process_ret_t mct_ret;
  serv_proc_ret_t ret;
  struct msm_v4l2_event_data *ret_data = (struct msm_v4l2_event_data *)
    ret.ret_to_hal.ret_event.u.data;

  read_len = read(fd, &mct_ret, sizeof(mct_process_ret_t));
  if(read_len <= 0) {
    ALOGE("%s: ERROR - read len is less than expected: %d", __func__, read_len);
    goto error;
  }

  ret.result = RESULT_SUCCESS;
  ret.ret_to_hal.ret = TRUE;

  switch(mct_ret.type) {
  ...
  case MCT_PROCESS_RET_ERROR_MSG: {
    ret.ret_to_hal.ret_type = SERV_RET_TO_HAL_NOTIFY_ERROR;
    ret_data->session_id    = mct_ret.u.bus_msg_ret.session;
  }
    break;
  default:
    break;
  }
  return ret;

error:
  ret.result = RESULT_FAILURE;
  ret.ret_to_hal.ret = FALSE;
  return ret;
}
