/*==========================================================
 * FUNCTION   : mm_channel_fsm_fn
 *
 * DESCRIPTION: channel finite state machine entry function. Depends on channel
 *				state, incoming event will be handled differently.
 *
 * PARAMETERS :
 *   @my_obj	: ptr to a channel object
 *   @evt       : channel event to be processed
 *	 @in_val	: input event payload. Can be NULL if not needed.
 *	 @out_val	: output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_channel_fsm_fn(mm_channel_t *my_obj,
						  mm_channel_evt_type_t evt,
						  void * in_val,
						  void * out_val)
{
	int32_t rc = -1;
	
	CDBG("%s : E state = %d", __func__, my_obj->state);
	switch(my_obj->state){
	....
	case MM_CHANNEL_STATE_STOPPED:
		rc = mm_channel_fsm_fn_stopped(my_obj, evt, in_val, out_val);
		break;
	....
	default:
		CDBG("%s: Not a valid state (%d)", __func__, my_obj->state);
		break;
	}
	
	/* unlock ch_lock */
	pthread_mutex_unlock(&my_obj->ch_lock);
	CDBG("%s : X rc = %d", __func__, rc);
	return rc;
}

/*==========================================================
 * FUNCTION   : mm_channel_fsm_fn_stopped
 *
 * DESCRIPTION: channel finite state machine function to handle event
 *				in STOPPED state.
 *
 * PARAMETERS :
 *   @my_obj	: ptr to a channel object
 *   @evt       : channel event to be processed
 *	 @in_val	: input event payload. Can be NULL if not needed.
 *	 @out_val	: output payload, Can be NULL if not needed.
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_channel_fsm_fn_stopped(mm_channel_t *my_obj,
								  mm_channel_evt_type_t evt,
								  void * in_val,
								  void * out_val)
{
	int32_t rc = 0;
	CDBG("%s : E evt = %d", __func__, evt);
	switch(evt){
	....
	case MM_CHANNEL_EVT_START:
	{
		rc = mm_channel_start(my_obj);
		/* first stream started in stopped state
		 * move to active state */
		if(rc == 0){
			my_obj->state = MM_CHANNEL_STATE_ACTIVE;
		}
	}
	break;
	....
	}/*switch(evt)*/
	CDBG("%s : E rc = %d", __func__, rc);
	return rc;
}

/*==========================================================
 * FUNCTION   : mm_channel_start
 *
 * DESCRIPTION: start a channel, which will start all streams in the channel
 *
 * PARAMETERS :
 *   @my_obj	: channel object
 *
 * RETURN     : int32_t type of status
 *              0 -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_channel_start(mm_channel_t *my_obj)
{
	int32_t rc = 0;
	int i, j;
	mm_stream_t *s_objs[MAX_STREAM_NUM_IN_BUNDLE] = {NULL};
	uint8_t num_streams_to_start = 0;
	mm_stream_t *s_obj = NULL;
	int meta_stream_idx = 0;
	cam_stream_type_t stream_type = CAM_STREAM_TYPE_DEFAULT;
	
	for(i = 0; i < MAX_STREAM_NUM_IN_BUNDLE; i++){
		if(my_obj->streams[i].my_hdl > 0){
			s_obj = mm_channel_util_get_stream_by_handler(my_obj,
														  my_obj->streams[i].my_hdl);
			if(NULL != s_obj){
				stream_type = s_obj->stream_info->stream_type;
				/* remember meta data stream index */
				if((stream_type == CAM_STREAM_TYPE_METADATA) &&
					(s_obj->ch_obj == my_obj)){
					meta_stream_idx = num_streams_to_start;	
				}
				s_objs[num_streams_to_start++] = s_obj;
			}
		}
	}
	
	if(meta_stream_idx > 0){
		/* always start meta data stream first, so switch the stream object with the first one */
		s_obj = s_objs[0];
		s_objs[0] = s_objs[meta_stream_idx];
		s_objs[meta_stream_idx] = s_obj;
	}
	
	if(NULL != my_obj->bundle.super_buf_notify_cb){
		/* need to send up cb, therefore launch thread */
		/* init superbuf queue */
		mm_channel_supperbuf_queue_init(&my_obj->bundle.superbuf_queue);
		my_obj->bundle.superbuf_queue.num_streams = num_streams_to_start;
		my_obj->bundle.superbuf_queue.expected_frame_id = 0;
		my_obj->bundle.superbuf_queue.expected_frame_id_without_led = 0;
		
		for(i = 0; i < num_streams_to_start; i++){
			/* Only bundle streams that belong to the channel */
			if(s_objs[i]->ch_obj == my_obj){
				/* set bundled flag to streams */
				s_objs[i]->is_bundled = 1;
			}
			/* init bundled streams to invalid value -1 */
			my_obj->bundle.superbuf_queue.bundled_stream[i] = s_objs[i]->my_hdl;
		}
		
		/* launch cb thread for dispatching super buf through cb */
		mm_camere_cmd_thread_launch(&my_obj->cb_thread,
									mm_channel_dispatch_super_buf,
									(void*)my_obj);
									
		/* launch cmd thread for super buf dataCB */
		mm_camera_cmd_thread_launch(&my_obj->cmd_thread,
									mm_channel_process_stream_buf,
									(void*)my_obj);
									
		/* set flag to TRUE */
		my_obj->bundle.is_active = TRUE;
	}
	
	for(i = 0; i < num_streams_to_start; i++){
		/* stream that are linked to this channel should not be started */
		if(s_objs[i]->ch_obj != my_obj){
			pthread_mutex_lock(&s_objs[i]->linked_stream->buf_lock);
			s_objs[i]->linked_stream->linked_obj = my_obj;
			s_objs[i]->linked_stream->is_linked = 1;
			pthread_mutex_unlock(&s_objs[i]->linked_stream->buf_lock);
			continue;
		}
		
		/*all streams within a channel should be started at the same time */
		if(s_objs[i]->state == MM_STREAM_STATE_ACTIVE){
			CDBG_ERROR("%s: stream already started idx(%d)", __func__, i);
			rc = -1;
			break;
		}
		
		/* allocate buf */
		rc = mm_stream_fsm_fn(s_objs[i], MM_STREAM_EVT_GET_BUF, NULL, NULL);
		if(0 != rc){
			CDBG_ERROR("%s: get buf failed at idx(%d)", __func__, i);
			break;
		}
		
		/* register buffer*/
		rc = mm_stream_fsm_fn(s_objs[i], MM_STREAM_EVT_REG_BUF, NULL, NULL);
		if(0 != rc){
			CDBG_ERROR("%s: reg buf failed at idx(%d)", __func__, i);
			break;
		}
		
		/* start stream */
		rc = mm_stream_fsm_fn(s_objs[i], MM_STREAM_EVT_START, NULL, NULL);
		if(0 != rc){
			CDBG_ERROR("%s: start stream failed at idx(%d)", __func__, i);
			break;
		}
	}
	
	/* error handling */
	if(0 != rc){
		for(j = 0; j <= i; j++){
			if(s_obj[j]->ch_obj != my_obj){
				/* Only unlink stream */
				pthread_mutex_lock(&s_objs[i]->linked_stream->buf_lock);
				s_objs[i]->linked_stream->is_linked = 0;
				s_objs[i]->linked_stream->linked_obj = NULL;
				pthread_mutex_unlock(&s_objs[i]->linked_stream->buf_lock);	
				
				if(TRUE == my_obj->bundle.is_active){
					mm_channel_superbuf_flush(my_obj, &my_obj->bundle.superbuf_queue, s_objs[j]->stream_info->stream_type);
				}
				memset(s_objs[j], 0, sizeof(mm_stream_t));
				
				continue;
			}
			
			/* stop streams */
			mm_stream_fsm_fn(s_objs[j], MM_STREAM_EVT_STOP, NULL, NULL);
			
			/* unreg buf */
			mm_stream_fsm_fn(s_objs[j], MM_STREAM_EVT_UNREG_BUF, NULL, NULL);
			
			/* put buf back */
			mm_stream_fsm_fn(s_objs[j], MM_STREAM_EVT_PUT_BUF, NULL, NULL);
		}
		
		/* destroy super buf cmd thread */
		if(TRUE == my_obj->bundle.is_active){
			/* first stop bundle thread */
			mm_camera_cmd_thread_release(&my_obj->cmd_thread);
			mm_camera_cmd_thread_release(&my_obj->cb_thread);
			
			/* deinit superbuf queue */
			mm_channel_superbuf_queue_deinit(&my_obj->bundle.superbuf_queue);
			
			/* memset bundle info */
			memset(&my_obj->bundle, 0, sizeof(mm_channel_bundle_t));
		}
	}
	
	return rc;
}
