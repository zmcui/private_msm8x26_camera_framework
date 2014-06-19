/* actuator.c
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/** af_actuator_load_params: loads the header params to the
 *  af driver
 * 	
 *  @ptr: pointer to actuator_data_t struct
 * 
 *  Return: 0 for sucess and negative error on failure
 *  
 *  This function returns 1 if af is supported, 0 otherwise **/
static int af_actuator_load_params(void *ptr)
{
	int rc = 0;
	struct msm_actuator_cfg_data cfg;
	actuator_data_t *af_actuator_ptr = (actuator_data_t *)ptr;
	uint8_t cnt = 0;
	uint16_t total_steps = 0;
	af_algo_tune_parms_t *af_tune_ptr = 
		af_actuator_ptr->ctrl->af_algo_ctrl;
	actuator_driver_params_t *af_driver_ptr =
		af_actuator_ptr->ctrl->driver_ctrl;
	actuator_tuned_params_t *actuator_tuned_params = NULL;
	actuator_params_t *actuator_params = NULL;
	
	if(af_actuator_ptr->is_af_supported){
		actuator_tuned_params = &af_driver_ptr->actuator_tuned_params;
		actuator_params = &af_driver_ptr->actuator_params;
		
		SERR("E");
		cfg.cfgtype = CFG_SET_ACTUATOR_INFO;
		total_steps = af_tune_ptr->af_algo.position_far_end + 1;
		total_steps += af_tune_ptr->af_algo.undershoot_adjust;
		af_actuator_ptr->total_steps = total_steps;
		cfg.cfg.set_info.af_tuning_params.total_steps = total_steps;
		cfg.cfg.set_info.actuator_params.act_type =
			actuator_params->act_type;
		cfg.cfg.set_info.af_tuning_params.pwd_step = 
			actuator_tuned_params->region_params[0].step_bound[1];
		cfg.cfg.set_info.af_tuning_params.initial_code =
			actuator_tuned_params->initial_code;
		cfg.cfg.set_info.actuator_params.reg_tbl_size = 
			actuator_params->reg_tbl.reg_tbl_size;
		cfg.cfg.set_info.actuator_params.reg_tbl_params = 
			&(actuator_params->reg_tbl.reg_params[0]);
		cfg.cfg.set_info.actuator_params.data_size = 
			actuator_params->data_size;
		cfg.cfg.set_info.actuator_params.i2c_addr = 
			actuator_params->i2c_addr;
		cfg.cfg.set_info.actuator_params.i2c_addr_type = 
			actuator_params->i2c_addr_type;
			
		cfg.cfg.set_info.af_tuning_params.region_size = 
			actuator_tuned_params->region_size;
		cfg.cfg.set_info.af_tuning_params.region_params = 
			&(actuator_tuned_params->region_params[0]);
		cfg.cfg.set_info.actuator_params.init_settting_size = 
			actuator_params->init_setting_size;
		cfg.cfg.set_info.actuator_params.i2c_data_type =
			actuator_params->i2c_data_type;
		cfg.cfg.set_info.actuator_params.init_settings = 
			&(actuator_params->inti_settings[0]);
			
		/* Invoke the IOCTL to set the af parameters to the kernel driver*/
		rc = ioctl(af_actuator_ptr->fd, VIDIOC_MSM_ACTUATOR_CFG, &cfg);
		if(rc < 0){
			SERR("failed rc %d", rc);
		}
	}
	return rc;
	
}
