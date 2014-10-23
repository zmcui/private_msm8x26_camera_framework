 /* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  * ....
  */


/*======================================================
 * FUNCTION   : mm_camera_set_parms
 *
 * DESCRIPTION: set parameters per camera
 *
 * PARAMETERS :
 *   @my_obj       : camera object
 *   @parms        : ptr to a param struct to be set to server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Corresponding fields of parameters to be set
 *              are already filled in by upper layer caller.
 *=========================================================*/
int32_t mm_camera_set_parms(mm_camera_obj_t *my_obj,
							parm_buffer_t *parms)
{
	int32_t rc = -1;
	int32_t value = 0;
	if(parms != NULL){
		rc = mm_camera_util_s_ctrl(my_obj->ctrl_fd, CAM_PRIV_PARM, &value);
	}
	pthread_mutex_unlock(&my_obj->cam_lock);
	return rc;
}


/*======================================================
 * FUNCTION   : mm_camera_util_s_ctrl
 *
 * DESCRIPTION: utility function to send v4l2 ioctl for s_ctrl
 *
 * PARAMETERS :
 *   @fd      : file descritpor for sending ioctl
 *   @id      : control id
 *   @value   : value of the ioctl to be sent
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *====================================================*/
int32_t mm_camera_util_s_ctrl(int32_t fd, uint32_t id, int32_t *value)
{
	int rc = 0;
	struct v4l2_control control;
	
	memset(&control, 0, sizeof(control));
	control.id = id;
	if(value != NULL){
		control.value = *value;
	}
	rc = ioctl(fd, VIDIOC_S_CTRL, &control);
	
	CDBG("%s: fd=%d, S_CTRL, id=0x%x, value = 0x%x, rc = %d\n",
		 __func__, fd, id, (uint32_t)value, rc);
	if(value != NULL){
		*value = control.value;
	}
	return (rc >= 0) ? 0 : -1;
}

/*======================================================
 * FUNCTION   : mm_camera_start_channel
 *
 * DESCRIPTION: start a channel, which will start all streams in the channel
 *
 * PARAMETERS :
 *   @my_obj  : camera object
 *   @ch_id   : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *====================================================*/
int32_t mm_camera_start_channel(mm_camera_obj_t *my_obj,
				uint32_t ch_id)
{
	int32_t rc = -1;
	mm_channel_t * ch_obj =
		mm_camera_util_get_channel_by_handler(my_obj, ch_id);
	
	if(NULL != ch_obj){
		pthread_mutex_lock(&ch_obj->ch_lock);
		pthread_mutex_unlock(&my_obj->cam_lock);
		
		rc = mm_channel_fsm_fn(ch_obj,
							   MM_CHANNEL_EVT_START,
							   NULL,
							   NULL);
	}else{
		pthread_mutex_unlock(&my_obj->cam_lock);
	}
	
	return rc;
}

