/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * ....
 */
 
 
/*==========================================================
 * FUNCTION   : mm_camera_intf_set_parms
 *
 * DESCRIPTION: set parameters per camera
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @parms        : ptr to a param struct to be set to server
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 * NOTE       : Assume the parms struct buf is already mapped to server via
 *              domain socket. Corresponding fields of parameters to be set
 *              are already filled in by upper layer caller.
 *=========================================================*/
static int32_t mm_camera_intf_set_parms(uint32_t camera_handle,
										parm_buffer_t *parms)
{
	int32_t rc = -1;
	mm_camera_obj_t * my_obj = NULL;
	
	pthread_mutex_lock(&g_intf_lock);
	my_obj = mm_camera_util_get_camera_by_handler(camera_handle);
	
	if(my_obj){
		pthread_mutex_lock(&my_obj->cam_lock);
		pthread_mutex_unlock(&g_intf_lock);
		rc = mm_camera_set_parms(my_obj, parms);
	}else{
		pthread_mutex_unlock(&g_intf_lock);
	}
	return rc;
}

/*==========================================================
 * FUNCTION   : mm_camera_intf_start_channel
 *
 * DESCRIPTION: start a channel, which will start all streams in the channel
 *
 * PARAMETERS :
 *   @camera_handle: camera handle
 *   @ch_id		   : channel handle
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              -1 -- failure
 *=========================================================*/
static int32_t mm_camera_intf_start_channel(uint32_t camera_handle,
											uint32_t ch_id)
{
	int32_t rc = -1;
	mm_camera_obj_t * my_obj = NULL;
	
	pthread_mutex_lock(&g_intf_lock);
	my_obj = mm_camera_util_get_camera_by_handler(camera_handle);
	
	if(my_obj){
		pthread_mutex_lock(&my_obj->cam_lock);
		pthread_mutex_unlock(&g_intf_lock);
		rc = mm_camera_start_channel(my_obj, ch_id);
	}else{
		pthread_mutex_unlock(&g_intf_lock);
	}
	CDBG("%s :X rc = %d", __func__, rc);
	return rc;
}


/* camera ops v-table */
/* czm this variable will be assigned with "cam_obj->vtbl.ops = &mm_camera_ops;" in camera_open() */
static mm_camera_ops_t mm_camera_ops = {
	.query_capability = mm_camera_intf_query_capability,
	.register_event_notify = mm_camera_intf_register_event_notify,
	.close_camera = mm_camera_intf_close,
	.set_parms = mm_camera_intf_set_parms,
	.get_parms = mm_camera_intf_get_parms,
	....
	.start_channel = mm_camera_intf_start_channel, //start a channel
	...
}



