/*============================================================
Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================*/
/* cpp_module_set_parm_sharpness:
 *
 **/
static int32_t cpp_module_set_parm_sharpness(cpp_module_ctrl_t *ctrl,
	uint32_t identity, int32_t value)
{
	....
	CDBG("%s:%d] value:%d, sharpness_level:%f\n", __func__, __LINE__, value,
	 session_params->hw_params.sharpness_level);
	 /* apply this to all streams in session */
	....
}

/* cpp_module_handle_set_parm_event:
 *
 * Description:
 *  Handle the set_parm event.
 **/
int32_t cpp_module_handle_set_parm_event(mct_module_t* module,
	mct_event_t* event)
{
	if(!module || !event){
		CDBG_ERROR("%s:%d, failed, module=%p, event=%p", __func__, __LINE__,
			module, event);
		return -EINVAL;
	}
	mct_event_control_parm_t *ctrl_parm = 
	(mct_event_control_parm_t *)event->u.ctrl_event.control_event_data;
	if(!ctrl){
		CDBG_ERROR("%s:%d, failed", __func__, __LINE__);
		return -EFAULT;
	}
	int32_t rc;
	switch(ctrl_parm->type){
	case CAM_INTF_PARM_SHARPNESS: {
		if(!(ctrl_parm->parm_data)){
			CDBG_ERROR("%s:%d, failed", __func__, __LINE__);
			return -EFAULT;
		}
		int32_t value = *(int32_t *)(ctrl_parm->parm_data);
		CDBG("%s:%d, CAM_INTF_PARM_SHARPNESS, value=%d, identity=0x%x",
		__func__, __LINE__, value, event->identity);
		rc = cpp_module_set_parm_sharpness(ctrl, event->identity, value);
		if(rc < 0){
			CDBG_ERROR("%s:%d, failed", __func__, __LINE__);
			return rc;
		}
		break;
	}
	....
	}
	
	rc = cpp_module_send_event_downstream(module, event);
	if(rc < 0){
		CDBG_ERROR("%s:%d, failed, module_event_type=%d, identity=0x%x",
		__func__, __LINE__, event->u.module_event.type, event->identity);
		return -EFAULT;
	}
	return 0;
}
