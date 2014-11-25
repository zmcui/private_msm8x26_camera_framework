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
	case MM_STREAM_STATE_BUFFED:
		rc = mm_stream_fsm_buffed(my_obj, evt, in_val, out_val);
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
	int32_t rc = 0;
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
 * FUNCTION   : mm_stream_fsm_buffed
 *
 * DESCRIPTION: stream finite state machine function to handle event in BUFFED
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
int32_t mm_stream_fsm_buffed(mm_stream_t *my_obj,
						mm_stream_evt_type_t evt,
						void * in_val,
						void * out_val)
{
	int32_t rc = 0;
	CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d",
		__func__, my_obj->my_hdl, my_obj->fd, my_obj->state);
	switch(evt){
	....
	case MM_STREAM_EVT_REG_BUF:
	  rc = mm_stream_reg_buf(my_obj);
	  /* change state to regged */
    if(rc = 0)
      my_obj->state = MM_STREAM_STATE_REG;
    break;
  ....
	}
	CDBG("%s :X rc = %d", __func__, rc);
	return rc;
}

/*======================================================
 * FUNCTION   : mm_stream_qbuf
 *
 * DESCRIPTION: enqueue buffer back to kernel queue for furture use
 *
 * PARAMETERS :
 *  @my_obj   : stream object
 *  @buf      : ptr to a struct storing buffer information
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_stream_qbuf(mm_stream_t *my_obj, mm_camera_buf_def_t *buf)
{
  int32_t rc = 0;
  struct v4l2_buffer buffer;
  struct v4l2_plane planes[VIDEO_MAX_PLANES];
  CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d, stream type = %d",
    __func__, my_obj->my_hdl, my_obj->fd, my_obj->state,
    my_obj->stream_info->stream_type);
  
  memcpy(planes, buf->planes, sizeof(planes));
  memset(&buffer, 0, sizeof(buffer));
  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  buffer.memory = V4L2_MEMORY_USERPTR;
  buffer.index = buf->buf_idx;
  buffer.m.planes = &planes[0];
  buffer.length = buf->num_planes;
  
  CDBG("%s:plane0: stream_hdl=%d, fd=%d, frame idx=%d, num_planes=%d, offset=%d, data_offset=%d\n", __func__,
    buf->stream_id, buf->fd, buffer.index, buffer.length, buf->planes[0].reserved[0], buf->planes[0].data_offset);
  CDBG("%s:plane1: stream_hdl=%d, fd=%d, frame idx=%d, num_planes=%d, offset=%d, data_offset=%d\n", __func__,
    buf->stream_id, buf->fd, buffer.index, buffer.length, buf->planes[1].reserved[0], buf->planes[1].data_offset);
    
  if(NULL != my_obj->mem_vtbl.invalidate_buf){
    rc = my_obj->mem_vtbl.invalidate_buf(buffer.index,
                                        my_obj->mem_vtbl.user_data)
    if(0 > rc){
      CDBG_ERROR("%s: cache invalidate failed on buffer index: %d", __func__, buffer.index);
      return rc;
    }
  }else{
    CDBG_ERROR("%s: Cache invalidate op not added", __func__);
  }
  
  my_obj->queued_buffer_count++;
  if(1 == my_obj->queued_buffer_count){
    /* Add fd to data poll thread */
    CDBG_HIGH("%s: Starting poll on stream %p type %d", __func__,
      my_obj, my_obj->stream_info->stream_type);
    rc = mm_camera_poll_thread_add_poll_fd(&my_obj->ch_obj->poll_thread[0],
          my_obj->my_hdl, my_obj->fd, mm_stream_data_notify, (void*)my_obj,
          mm_camera_async_call);
    if(0 > rc){
      CDBG_ERROR("%s: Add poll on stream %p type: %d fd error (rc=%d)",
          __func__, my_obj, my_obj->stream_info->stream_type, rc);
    }else{
      CDBG_HIGH("%s: Started poll in stream %p type: %d", __func__,
        my_obj, my_obj->stream_info->stream_type);
    }
  }
  CDBG("%s: VIDIOC_QBUF:fd = %d, state = %d stream_type = %d, qbuf_index %d, frame_idx %d",
    __func__, my_obj->fd, my_obj->state, my_obj->stream_info->stream_type,
    buffer.index, buf->frame_idx);
    
  rc = ioctl(my_obj->fd, VIDIOC_QBUF, &buffer);
  if(0 > rc){
    CDBG_ERROR("%s: VIDIOC_QBUF ioctl call failed in stream type %d (rc = %d): %s",
      __func__, my_obj->stream_info->stream_type, rc, strerror(errno));
    my_obj->queued_buffer_count--;
    if(0 == my_obj->queued_buffer_count){
      /*Removed fd from data poll in case of failing
       * first buffer queueing attempt */
      CDBG_HIGH("%s: Stoping poll in stream %p type: %d", __func__,
        my_obj, my_obj->stream_info->stream_type);
      mm_camera_poll_thread_del_fd(&my_obj->ch_obj->poll_thread[0],
        my_obj->my_hdl, mm_camera_async_call);
      CDBG_HIGH("%s: Stoping poll in stream %p type: %d", __func__,
        my_obj, my_obj->stream_info->stream_type);
    }
  }else{
    CDBG_HIGH("%s: VIDIOC_QBUF buf_index %d, stream type %d, frame_idx %d, queued cnt %d",
        __func__, buffer.index,
        my_obj->stream_info->stream_type,
        buf->frame_idx, my_obj->queued_buffer_count);
  }
  
  return rc;
}

/*======================================================
 * FUNCTION   : mm_stream_request_buf
 *
 * DESCRIPTION: This function let kernel know the amount of buffers need to
 *              be registered via v4l2 ioctl.
 *
 * PARAMETERS :
 *  @my_obj   : stream object
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_stream_request_buf(mm_stream_t * my_obj)
{
  int32_t rc = 0;
  struct v4l2_requestbuffers bufreq;
  uint8_t buf_num = my_obj->buf_num;
	CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d",
		__func__, my_obj->my_hdl, my_obj->fd, my_obj->state);
		
	if(buf_num > MM_CAMERA_MAX_NUM_FRAMES){
	  CDBG_ERROR("%s: buf_num %d > max limit %d\n",
	      __func__, buf_num, MM_CAMERA_MAX_NUM_FRAMES);
	  return -1;
	}
  
  memset(&bufreq, 0, sizeof(bufreq));
  bufreq.count = buf_num;
  bufreq.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
  bufreq.memory = V4L2_MEMORY_USERPTR;
  rc = ioctl(my_obj->fd, VIDIOC_REQBUFS, &bufreq);
  if(rc < 0){
    CDBG_ERROR("%s: fd=%d, ioctl VIDIOC_REQBUFS failed: rc = %d\n", 
      __func__, my_obj->fd, rc);
  }
  
  CDBG("%s: X rc = %d", __func__, rc);
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

/*======================================================
 * FUNCTION   : mm_stream_reg_bufs
 *
 * DESCRIPTION: register buffers with kernel by calling v4l2 ioctl QBUF for
 *		each buffer in the stream
 *
 * PARAMETERS :
 *  @my_obj   : stream object
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
int32_t mm_stream_reg_buf(mm_stream_t * my_obj)
{
  int32_t rc = 0;
  uint8_t i;
  CDBG("%s: E, my_handle = 0x%x, fd = %d, state = %d",
    __func__, my_obj->my_hdl, my_obj->fd, my_obj->state);
    
  rc = mm_stream_request_buf(my_obj);
  if(rc != 0){
    return rc;  
  }
  
  pthread_mutex_lock(&my_obj->buf_lock);
	my_obj->queued_buffer_count = 0;
	for(i = 0; i < my_obj->buf_num; i++){
	  /* check if need to qbuf initially */
	  if(my_obj->buf_status[i].initial_reg_flag){
	    rc = mm_stream_qbuf(my_obj, &my_obj->buf[i]);
	    if(rc != 0){
	      CDBG_ERROR("%s: VIDIOC_QBUF rc = %d\n", __func__, rc);
	      break;
	    }
	    my_obj->buf_status[i].buf_refcnt = 0;
	    my_obj->buf_status[i].in_kernel = 1;
	  }else{
	    /* the buf is held by upper layer, will not queue into kernel.
	     * add buf reference count */
	    my_obj->buf_status[i].buf_refcnt = 0;
	    my_obj->buf_status[i].in_kernel = 1;    
	  }
	}
	pthread_mutex_unlock(&my_obj->buf_lock);
	
	return rc;
}

