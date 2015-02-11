/** mct_controller_new:
 *  @mods: modules list
 *  @session_idx: session index
 *  @serv_fd: file descriptor for MCT to communicate
 *          back to imaging server
 *
 *  create a new Media Controller object. This is a
 *  new session pipeline
 *
 *  This function executes in Server context
 **/
boolean mct_controller_new(mct_list_t *mods,
    unsigned int session_idx, int serv_fd)
{
  mct_controller_t *mct = NULL;
  int              ds_fd;
  pthread_t        tid;

  mct = (mct_controller_t *)malloc(sizeof(mct_controller_t));
  if (!mct)
    goto mct_error;

  mct->pipeline = mct_pipeline_new();
  if(!mct->pipeline)
    goto pipeline_error;

  mct->pipeline->modules = mods;
  mct->pipeline->session = session_idx;

  mct_pipeline_start_session(mct->pipeline);

  ....
  pthread_mutex_lock(&mct->mctl_thread_started_mutex);

  if(pthread_create(&tid, NULL, mct_controller_thread_run, mct)){
    pthread_mutex_unlock(&mct->mctl_thread_started_mutex);
    goto thread_error;
  }

  pthread_cond_wait(&mct->mctl_thread_started_cond,
      &mct->mctl_thread_started_mutex);
  pthread_mutex_unlock(&mct->mctl_thread_started_mutex);
  ...
}

/** mct_controller_proc_servmsg:
 *  @servMsg: the message to be posted
 *
 *  Post imaging server message to Media Controller's message
 *  message queue.
 *
 *  This function executes in Imaging Server context
 **/
boolean mct_controller_proc_serv_msg(mct_serv_msg_t *serv_msg)
{
	mct_controller_t *mct;
	mct_list_t       *mct_list;
	mct_serv_msg_t   *msg;
	unsigned int     session;
	
	switch (serv_msg->msg_type) {
	case SERV_MSG_DS:
		session = serv_msg->u.ds_msg.session;
		break;
	
	case SERV_MSG_HAL: {
		struct msm_v4l2_event_data *data =
		(struct msm_v4l2_event_data *)(serv_msg->u.hal_msg.u.data);
		session = data->session_id;
	}
		break;
	
	default:
		return FALSE;
	}
	
	....
	/* Push message to Media Controller Message Queue
	* and post signal to Media Controller */
	pthread_mutex_lock(&mct->serv_msg_q_lock);
	mct_queue_push_tail(mct->serv_cmd_q, msg);
	pthread_mutex_unlock(&mct->serv_msg_q_lock);
	
	pthread_mutex_lock(&mct->mctl_mutex);
	mct->serv_cmd_q_counter++;
	pthread_cond_signal(&mct->mctl_cond);
	pthread_mutex_unlock(&mct->mctl_mutex);
	
	return TRUE;
}


/** mct_controller_proc_servmsg_internal:
 *	@mct: Media Controller Object
 *	@msg: message object from imaging server
 *	
 *	Media Controller process Imaging Server messages
 *	Return: mct_process_ret_t
 *
 *	This function executes in Media Controller's thread context
 **/
static mct_process_ret_t mct_controller_proc_serv_msg_internal(
mct_controller_t *mct, mct_serv_msg_t *msg)
{
	mct_proc_ret ret;
	mct_pipeline_t *pipeline;
	
	memset(&ret, 0x00, sizeof(mct_process_ret_t));
	ret.type = MCT_PROCESS_RET_SERVER_MSG;
	ret.u.serv_msg_ret.error = TRUE;
	
	if(!mct || !msg || !mct->pipeline){
		ret.u.serv_msg_ret.error = TRUE;
		return ret;
	}
	
	switch(msg->msg_type){
	case SERV_MSG_DS:{
		if(msg->u.ds_msg.operation == CAM_MAPPING_TYPE_FD_MAPPING &&
		   pipeline->map_buf){
		ALOGV("[dbgHang] Map buffer >>> enter");
		ret.u.serv_msg_ret.error = pipeline->map_buf(&msg->u.ds_msg, pipeline);
		ALOGV("[dbgHang] Map buffer >>> exit with status: %d", ret.u.serv_msg_ret.error);
		}else if(msg->u.ds_msg.operation == CAM_MAPPING_TYPE_FD_UNMAPPING &&
		   pipeline->unmap_buf){
		ALOGV("[dbgHang] UnMap buffer >>> enter");
		ret.u.serv_msg_ret.error = pipeline->unmap_buf(&msg->u.ds_msg, pipeline);
		ALOGV("[dbgHang] UnMap buffer >>> exit with status: %d", ret.u.serv_msg_ret.error);
		}
	}
		break;
	
	case SERV_MSG_HAL:
		if (pipeline->process_serv_msg)
		ret.u.serv_msg_ret.error = pipeline->process_serv_msg(&msg->u.hal_msg, pipeline);
		break;
		
	default:
		break;
	}
	
	return ret;
}

/** mct_controller_proc_bus_msg_internal:
 *	Media Controller process Bus message
 *
 *	@mct: Media Controller Object
 *	@msg: message object from bus
 *	
 *	Return: mct_process_ret_t
 *
 *	This function executes in Media Controller's thread context
 **/
static mct_process_ret_t mct_controller_proc_bus_msg_internal(
    mct_controller_t *mct, mct_bus_msg_t *bus_msg)
{
  mct_process_ret_t ret;
  mct_pipeline_t *pipeline;

  ret.u.bus_msg_ret.error = TRUE;
  ret.type = MCT_PROCESS_RET_BUS_MSG;

  if(!mct || !bus_msg || !mct->pipeline){
    return ret;
  }

  if(!mct_controller_check_pipeline(mct->pipeline)) {
    return ret;
  }

  ret.u.bus_msg_ret.error = FALSE;
  ret.u.bus_msg_ret.msg_type = bus_msg->type;
  ret.u.bus_msg_ret.session = bus_msg->sessionid;

  if (bus_msg->type == MCT_BUS_MSG_SEND_HW_ERROR){
    ret.type = MCT_PROCESS_RET_ERROR_MSG;
    return ret;
  }
  ...
}

/** mct_controller_thread_run:
 *	@data: structure of mct_controller_t
 *
 *	Media Controller Thread
 **/
static void* mct_controller_thread_run(void *data)
{
	mct_controller_t 	*mct_this;
	mct_process_ret_t 	proc_ret;
	mct_serv_msg_t		*msg;
	mct_bus_msg_t		*bus_msg;
	
	....
	
	do{
		/* First make sure there aren't any pending events in the queue. This
		   is required since if commands come from multiple threads for the
		   same session, it may happen the signal was made while this thread
		   was busy processing another command, in which case we may
		   lose out on this new command */
		pthead_mutex_lock(&mct_this->serv_msg_q_lock);
		msg = (mct_serv_msg_t *)mct_queue_pop_head(mct_this->serv_cmd_q);
		pthead_mutex_unlock(&mct_this->serv_msg_q_lock);
		
		if(msg){
			pthead_mutex_lock(&mct_this->mctl_mutex);
			mct_this->serv_cmd_q_counter--;
			pthead_mutex_unlock(&mct_this->mctl_mutex);
			
			proc_ret = mct_controller_proc_serv_msg_internal(mct_this, msg);
			free(msg);
			
			if(proc_ret.type == MCT_PROCESS_RET_SERVER_MSG				&&
			   proc_ret.u.serv_msg_ret.msg.msg_type == SERV_MSG_HAL		&&
			   proc_ret.u.serv_msg_ret.msg.u.hal_msg.id == MSM_CAMERA_DEL_SESSION){
			  goto close_mct;
			}
			
			/* Based on process result, need to send event to server */
			write(mct_this->serv_fd, &proc_ret, sizeof(mct_process_ret_t));
			
			// Try to pop anther message from the queue
			continue;

      pthread_mutex_lock(&mct_this->mctl_mutex);

      tmp_bus_q_cmd = mct_this->pipeline->bus->bus_cmd_q_flag;
      mct_this->pipeline->bus->bus_cmd_q_flag = FALSE;
      if(!tmp_bus_q_cmd && !mct_this->serv_cmd_q_counter) {
        pthread_cond_wait(&mct_this->mctl_cond, &mct_this->mctl_mutex);
      }

      pthread_mutex_unlock(&mct_this->mctl_mutex);

      /* Received Signal from Pipeline Bus */
      while(tmp_bus_q_cmd) {
        pthread_mutex_lock(&mct_this->pipeline->bus->bus_msg_q_lock);
        bus_msg = (mct_bus_msg_t *)mct_queue_pop_head(mct_this->pipeline->bus->bus_queue);
        pthread_mutex_unlock(&mct_this->pipeline->bus->bus_msg_q_lock);

        if(!bus_msg) {
          break;
        }
        proc_ret = mct_controller_proc_bus_msg_internal(mct_this, bus_msg);
        
        if(bus_msg->msg)
          free(bus_msg->msg);

        if(bus_msg)
          free(bus_msg);

        if(proc_ret.type == MCT_PROCESS_RET_ERROR_MSG ||
            (proc_ret.type = MCT_PROCESS_RET_BUS_MSG &&
             proc_ret.u.bus_msg_ret.msg_type == MCT_BUS_MSG_REPROCESS_STAGE_DONE) ||
            (proc_ret.type == MCT_PROCESS_RET_BUS_MSG &&
             proc_ret.u.bus_msg_ret.msg_type == MCT_BUS_MSG_SEND_EZTUNE_EVT)){
          write(mct_this->serv_fd, &proc_ret, sizeof(mct_process_ret_t));
        }
    }
	}while(1);
close_mct:
	return NULL;
}
