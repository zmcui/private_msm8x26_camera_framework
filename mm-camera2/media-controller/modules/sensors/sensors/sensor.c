/* sensor.c
 *
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
 
/*=============================================
 * FUNCTION    - sensor_set_sharpness -
 *
 * DESCRIPTION:
 *===========================================*/
static int32_t sensor_set_sharpness(void *sctrl, void *data)
{
	int32_t ret = 0;
	//struct msm_camera_i2c_reg_array set_config;
	struct sensorb_cfg_data cfg;
	sensor_ctrl *ctrl = (sensor_ctrl_t *)sctrl;
	int32_t *sharpness_level = (int32_t*)data;
	SHIGH("%s: SHARPNESS VALUE %d", __func__, *sharpness_level);
	cfg.cfg.setting = sharpness_level;
	cfg.cfgtype = CFG_SET_SHARPNESS;
	
	ret = ioctl(ctrl->s_data->fd, VIDIOC_MSM_SENSOR_CFG, &cfg);
	if(ret < 0){
		SERR("failed");
		return ret;
	}
	return ret;
}

/*=============================================
 * FUNCTION    - sensor_process -
 *
 * DESCRIPTION:
 *===========================================*/
static int32_t sensor_process(void *sctrl,
	sensor_submodule_event_type_t event, void *data)
{
	int32_t rc = SENSOR_SUCCESS;
	if(!sctrl){
		SERR("failed");
		return SENSOR_FAILURE;
	}
	SLOW("sctrl %p event %d", sctrl, event);
	switch(event){
	/*Get enums*/
	....
	case SENSOR_SET_SHARPNESS:
	  rc = sensor_set_sharpness(sctrl, data);
	  break;
	....
	}
}
 
int32_t sensor_sub_module_init(senser_func_tbl_t *func_tbl)
{
	if(!func_tbl){
		SERR("failed");
		return SENSOR_FAILURE;
	}
	func_tbl->open = sensor_open;
	func_tbl->process = sensor_process;
	func_tbl->close = sensor_close;
	return SENSOR_SUCCESS;
}
