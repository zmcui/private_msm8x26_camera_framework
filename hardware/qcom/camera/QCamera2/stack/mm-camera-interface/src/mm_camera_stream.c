 /* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  * ....
  */

/*======================================================
 * FUNCTION   : mm_stream_fsm_fn
 *
 * DESCRIPTION: stream finite state machine entry function. Depends on stream 
 *				state, incoming event will be handled differently.
 *
 * PARAMETERS :
 *  @my_obj     :ptr to a stream object
 *	@evt		:stream event to be precessed
 *	@in_val		:input event payload. Can be NULL if not needed.
 *	@out_val	:output payload, Can be NULL if not needed
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_stream_fsm_fn(mm_stream_t *my_obj,
						mm_stream_evt_type_t evt,
						void * in_val,
						void * out_val)
{
	int32_t rc = -1;
	
	CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d",
		__func__, my_obj->my_hdl, my_obj->fd, my_obj->state);
	switch(my_obj->state){
	....
	case MM_STREAM_STATE_CFG:
		rc = mm_stream_fsm_cfg(my_obj, evt, in_val, out_val);
	....
	}
	CDBG("%s : X rc =%d",__func__,rc);
	return rc;
}

/*======================================================
 * FUNCTION   : mm_stream_fsm_cfg
 *
 * DESCRIPTION: stream finite state machine function to handle event in CONFIGURED
 *				state
 *
 * PARAMETERS :
 *  @my_obj     :ptr to a stream object
 *	@evt		:stream event to be precessed
 *	@in_val		:input event payload. Can be NULL if not needed.
 *	@out_val	:output payload, Can be NULL if not needed
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_stream_fsm_cfg(mm_stream_t *my_obj,
						mm_stream_evt_type_t evt,
						void * in_val,
						void * out_val)
{
	int32_t re = 0;
	CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d",
		__func__, my_obj->my_hdl, my_obj->fd, my_obj->state);
	switch(evt){
	....
	case MM_STREAM_EVT_GET_BUF:
		rc = mm_stream_init_bufs(my_obj);
		/* change state to buff allocated */
		if(0 == rc){
			my_obj->state = MM_STREAM_STATE_BUFFED;
		}
		break;
		....
	}
	CDBG("%s :X rc = %d", __func__, rc);
	return rc;
}

/*======================================================
 * FUNCTION   : mm_stream_init_bufs
 *
 * DESCRIPTION: initialize stream buffers needed. This function will request
 *				buffers needed from upper layer through the mem ops table passed
 *				during configuration stage.
 *
 * PARAMETERS :
 *  @my_obj     :ptr to a stream object
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_stream_init_bufs(mm_stream_t *my_obj)
{
	int32_t i, rc = 0;
	uint8_t *reg_flags = NULL;
	mm_camera_map_unmap_ops_tbl_t ops_tbl;
	CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d",
		__func__, my_obj->my_hdl, my_obj->fd, my_obj->state);
		
	/* deinit buf if it's not NULL */
	if(NULL != my_obj->buf){
		mm_stream_deinit_bufs(my_obj);
	}
	
	ops_tbl.map_ops = mm_stream_map_buf_ops;
	ops_tbl.unmap_ops = mm_stream_unmap_buf_ops;
	ops_tbl.userdata = my_obj;
	
	rc = my_obj->mem_vtbl.get_bufs(&my_obj->frame_offset,
									&my_obj->buf_num,
									&reg_flags,
									&my_obj->buf,
									&ops_tbl,
									my_obj->mem_vtbl.user_data);
	
	if(0 != rc){
		CDBG_ERROR("%s: Error get buf, rc = %d\n", __func__, rc);
		return rc;
	}

	my_obj->buf_status =
		(mm_stream_buf_status_t *)malloc(sizeof(mm_stream_buf_status) * my_obj->buf_num);
	
	if(NULL == my_obj->buf_status){
		CDBG_ERROR("%s: No memory for buf_status", __func__);
		mm_stream_deinit_buf(my_obj);
		free(reg_flags);
		return -1;
	}
	
	memset(my->obj->buf_status, 0, sizeof(mm_stream_buf_status) * my_obj->buf_num);
	for(i = 0; i < my_obj->buf_num; i++){
		my_obj->buf_status[i].initial_reg_flag = reg_flags[i];
		my_obj->buf[i].stream_id = my_obj->my_hdl;
		my_obj->buf[i].stream_type = my_obj->stream_info->stream_type;
	}
	
	free(reg_flags);
	reg_flags = NULL;
	
	/* update in stream info about number of stream buffers */
	my_obj->stream_info->num_bufs = my_obj->buf_num;
	
	return rc;
}
