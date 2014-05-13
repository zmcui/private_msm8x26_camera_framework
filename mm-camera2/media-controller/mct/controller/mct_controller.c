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
	....
	case SERV_MSG_HAL:
		if (pipeline->process_serv_msg)
		ret.u.serv_msg_ret.error = pipeline->process_serv_msg(&msg->u.hal_msg, pipeline);
		break;
	....
}
