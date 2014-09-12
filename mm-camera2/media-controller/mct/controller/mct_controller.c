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
		}
		
		....
	}while(1);
close_mct:
	return NULL;
}
