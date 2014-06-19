/* Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
static int32_t msm_actuator_set_param(struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_set_info_t *set_info)
{
	struct reg_setting_t *init_settings = NULL;
	int32_t rc = -EFAULT;
	uint16_t i = 0;
	struct msm_camera_cci_client *cci_client = NULL;
	CDBG("Enter\n");
	
	for(i = 0; i < ARRAY_SIZE(actuators); i++){
		if(set_info->actuator_params.act_type ==
		  actuators[i]->act_type) {
		  	a_ctrl->func_tbl = &actuator[i]->func_tbl;
		  	rc = 0;
		  }
	}
	
	if(rc < 0){
		pr_err(""Actuator function table not found\n);
		return rc;
	}
	if(set_info->af_tuning_params.total_steps
		> MAX_ACTUATOR_AF_TOTAL_STEPS){
		pr_err("Max actuator total steps exceeded = %d\n",
		set_info->af_tuning_params.total_steps);
		return -EFAULT;
	}
	if(set_info->af_tuning_params.region_size
	> MAX_ACTUATOR_REGION){
		pr_err("MAX_ACTUATOR_REGION is exceeded.\n");
		return -EFAULT;
	}
	
	a_ctrl->region_size = set_info->af_tuning_params.region_size;
	a_ctrl->pwd_step = set_info->af_tuning_params.pwd_step;
	a_ctrl->total_steps = set_info->af_tuning_params.total_steps;
	
	if(copy_from_user(&a_ctrl->region_params,
		(void *)set_info->af_tuning_params.region_params,
		a_ctrl->region_size*sizeof(struct region_params_t)))
		return -EFAULT;
		
	if(a_ctrl->act_device_type == MSM_CAMERA_PLATFORM_DEVICE){
		cci_client = a_ctrl->i2c_client.cci_client;
		cci_client->sid =
			set_info->actuator_params.i2c_addr >> 1;
		cci_client->retries = 3;
		cci_client->id_map = 0;
		cci_client->cci_i2c_master = a_ctrl->cci_master;
	}else{
		a_ctrl->i2c_client.client->addr = 
			set_info->actuator_params.i2c_addr;
	}
	
	a_ctrl->i2c_data_type = set_info->actuator_params.i2c_data_type;
	a_ctrl->i2c_client.addr_type = set_info->actuator_params.i2c_add_type;
	if(set_info->actuator_params.reg_tbl_size <=
		MAX_ACTUATOR_REG_TBL_SIZE){
		a_ctrl->reg_tbl_size = set_info->actuator_params.reg_tbl_size;	
	}else{
		a_ctrl->reg_tbl_size = 0;
		pr_err("MAX_ACTUATOR_REG_TBL_SIZE is exceeded.\n");
		return -EFAULT;
	}
	
	kfree(a_ctrl->i2c_reg_tbl);
	a_ctrl->i2c_reg_tbl = NULL;
	a_ctrl->i2c_reg_tbl =
		kmalloc(sizeof(struct msm_camera_i2c_reg_array) *
		(set_info->af_tuning_params.total_steps + 1), GFP_KERNEL);
	if(!a_ctrl->i2c_reg_tbl){
		pr_err("kmalloc fail\n");
		return -ENOMEM;
	}
	
	if(copy_from_user(&a_ctrl->reg_tbl,
		(void *)set_info->actuator_params.reg_tbl_params,
		a_ctrl->reg_tbl_size *
		sizeof(struct msm_actuator_reg_params_t))){
		kfree(a_ctrl->i2c_reg_tbl);
		a_ctrl->i2c_reg_tbl = NULL;
		return -EFAULT;
	}
	
	if(set_info->actuator_params.init_setting_size &&
	   set_info->actuator_params.init_setting_size
	   <= MAX_ACTUATOR_REG_TBL_SIZE){
	   	if(a_ctrl->func_tbl->actuator_init_focus){
	   		init_setting = kmalloc(sizeof(struct reg_setting_t) *
	   		(set_info->actuator_params.init_setting_size),
	   		GFP_KRENEL);
	   		if(init_setting == NULL){
	   			kfree(a_ctrl->i2c_reg_tbl);
	   			a_ctrl->i2c_reg_tbl = NULL;
	   			pr_err("Error allocating memory for init_setting\n");
	   			return -EFAULT;
	   		}
	   		if(copy_from_user(init_setting,
	   		(void *)set_info->actuator_params.init_settings,
	   		set_info->actuator_params.init_setting_size *
	   		sizeof(struct reg_setting_t))){
	   			kfree(init_setting);
	   			kfree(a_ctrl->i2c_reg_tbl);
	   			a_ctrl->i2c_reg_tbl = NULL;
	   			pr_err("Error copying init_settings\n");
	   			return -EFAULT;
	   		}
	   		rc = a_ctrl->func_tbl->actuator_init_focus(a_ctrl,
	   			set_info->actuator_params.init_setting_size,
	   			init_settings);
	   		kfree(init_settings);
	   		if(rc < 0){
	   			kfree(a_ctrl->i2c_reg_tbl);
	   			a_ctrl->i2c_reg_tbl = NULL;
	   			pr_err("Error actuator_init_focus\n");
	   			return -EFAULT;
	   		}
	   	}
	   }
	   
	   a_ctrl->initial_code = set_info->af_tuning_params.initial_code;
	   if(a_ctrl->func_tbl->actuator_init_step_table)
	   		rc = a_ctrl->func_tbl->
	   			actuator_init_step_table(a_ctrl, set_info);

	   a_ctrl->curr_step_pos = 0;
	   a_ctrl->curr_region_index = 0;
	   a_ctrl->actuator_state = ACTUATOR_POWER_UP;
	   CDBG("Exit\n");
	   
	   return rc;
}
 

static int32_t msm_actuator_config(struct msm_actuator_ctrl_t *a_ctrl,
	void __user *argp)
{
	struct msm_actuator_cfg_data *cdata =
		(struct msm_actuator_cfg_data *)argp;
	int32_t rc = 0;
	mutex_lock(a_ctrl->actuator_mutex);
	CDBG("Enter\n");
	CDBG("%s type %d\n", __func__, cdata->cfgtype);
	switch(cdata->cfgtype){
	case CFG_ACTUATOR_INIT:
		....
	case CFG_GET_ACTUATOR_INFO:
		....
	case CFG_SET_ACTUATOR_INFO:
		rc = msm_actuator_set_param(a_ctrl, &cdata->cfg.set_info);
		if(rc < 0)
			pr_err("init table failed %d\n", rc);
		break;
	....
	}
	mutex_unlock(a_ctrl->actuator_mutex);
	CDBG("Exit\n");
	return rc;
}
 
static long msm_actuator_subdev_ioctl(struct v4l2_subdev *sd,
		unsigned int cmd, void *arg)
{
	struct msm_actuator_ctrl_t *a_ctrl = v4l2_get_subdevdata(sd);
	void __user *argp = (void __user *)arg;
	CDBG("Enter\n");
	CDBG("%s:%d a_ctrl %p argp %p\n", __func__, __LINE__, a_ctrl, argp);
	switch(cmd){
	case VIDIOC_MSM_SENSOR_GET_SUBDEV_ID:
		return msm_actuator_get_subdev_id(a_ctrl, argp);
	case VIDIOC_MSM_ACTUATOR_CFG:
		return msm_actuator_config(a_ctrl, argp);
	case MSM_SD_SHUTDOWN:
		msm_actuator_close(sd, NULL);
		return 0;
	default:
		return -ENOIOCTLCMD;
	}
}

static struct v4l2_subdev_core_ops msm_actuator_subdev_core_ops = {
	.ioctl = msm_actuator_subdev_ioctl,	
	.s_power = msm_actuator_power,
}

static struct v4l2_subdev_ops msm_actuator_subdev_ops = {
	.core = &msm_actuator_subdev_core_ops,	
}

static int32_t msm_actuator_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	struct msm_camera_cci_client *cci_client = NULL;
	struct msm_actuator_ctrl_t *msm_actuator_t = NULL;
	struct msm_actuator_vreg *vreg_cfg;
	CDBG("Enter\n");
	
	....
	msm_actuator_t->act_v4l2_subdev_ops = &msm_actuator_subdev_ops;
	....
	v4l2_set_subdedata(&msm_actuator_t->msm_sd.sd, msm_actuator_t);
	....
}

static int __init msm_actuator_init_module(void)
{
	int32_t rc = 0;
	CDBG("Enter\n");
	rc = platform_driver_probe(&msm_actuator_platform_driver,
		msm_actuator_platform_probe);
	if(!rc)
		return rc;
	CDBG("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&msm_actuator_i2c_driver);
}

module_init(msm_actuator_init_module);
MODULE_DESCRIPTION("MSM ACTUATOR");
MODULE_LICENSE("GPL v2");
