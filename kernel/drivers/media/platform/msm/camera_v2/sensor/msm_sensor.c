/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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
static struct msm_sensor_fn_t msm_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
 	.sensor_power_down = msm_sensor_power_down,
 	.sensor_match_id = msm_sensor_match_id,
}
 
static struct msm_camera_i2c_fn_t msm_sensor_cci_func_tbl = {
	.i2c_read = msm_camera_cci_i2c_read,
	.i2c_read_seq = msm_camera_cci_i2c_read_seq,
	.i2c_write = msm_camera_cci_i2c_write,
	.i2c_write_table = msm_camera_cci_i2c_write_table;
	.i2c_write_seq_table = msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay = msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util = msm_sensor_cci_i2c_util,
	.i2c_write_conf_tbl = msm_camera_cci_i2c_write_conf_tbl,
}
 
int32_t msm_sensor_platform_probe(struct platform_device *pdev, void *data)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl =
		(struct msm_sensor_ctrl_t *)data;
	struct msm_camera_cci_client *cci_client = NULL;
	uint32_t session_id;
	unsigned long mount_pos;
	
	s_ctrl->pdev = pdev;
	s_ctrl->dev &pdev->dev;
	CDBG("%s called data %p\n", __func__, data);
	CDBG("%s pdev name %s\n", __func__, pdev->id_entry->name);
	if(pdev->dev.of_node){
		rc = msm_sensor_get_dt_data(pdev->dev.of_node, s_ctrl);
		if(rc < 0){
			pr_err("%s failed line %d\n", __func__, __LINE__);
			return rc;
		}
	}
	s_ctrl->sensor_device_type = MSM_CAMERA_PLATFORM_DEVICE;
	s_ctrl->sensor_i2c_client->cci_client = kzalloc(sizeof(struct msm_camera_cci_client), GFP_KERNEL);
	if(!s_ctrl->sensor_i2c_client->cci_client){
		pr_err("%s failed line %d\n", __func__, __LINE__);
		return rc;
	}
	/* TODO: get CCI subdev */
	cci_client = s_ctrl->sensor_i2c_client->cci_client;
	cci_client->cci_subdev = msm_cci_get_subdev();
	cci_client->cci_i2c_master = s_ctrl->cci_i2c_master;
	cci_client->sid = s_ctrl->sensordata->slave_info->sensor_slave_addr >> 1;
	cci_client->retries = 3;
	cci_client->id_map = 0;
	if(!s_ctrl->func_tbl)
		s_ctrl->func_tbl = &msm_sensor_func_tbl;
	if(!s_ctrl->sensor_i2c_client->i2c_func_tbl)
		s_ctrl->sensor_i2c_client->i2c_func_tbl =
		&msm_sensor_cci_func_tbl;
	if(!s_ctrl->sensor_v4l2_subdev_ops)
		s_ctrl->sensor_v4l2_subdev_ops = &msm_sensor_subdev_ops;
	s_ctrl->clk_info = kzalloc(sizeof(cam_8974_clk_info), GFP_KERNEL);
	if(!s_ctrl->clk_info){
		pr_err("%s:%d failed nomem\n", __func__, __LINE__);
		kfree(cci_client);
		return -ENOMEM;
	}
	memcpy(s_ctrl->clk_info, cam_8974_clk_info, sizeof(cam_8974_clk_info));
	s_ctrl->clk_info_size = ARRAY_SIZE(cam_8974_clk_info);
	if(rc < 0){
		pr_err("%s %s power up failed\n", __func__, s_ctrl->sensordata->sensor_name);
		kfree(s_ctrl->clk_info);
		kfree(cci_client);
		return rc;
	}
	
	CDBG("%s %s probe succeeded\n", __func__, s_ctrl->sensordata->sensor_name);
	v4l2_subdev_init(&s_ctrl->msm_sd.sd, s_ctrl->sensor_v4l2_subdev_ops);
	snprintf(s_ctrl->msm_sd.sd.name, sizeof(s_ctrl->msm_sd.sd.name), "%s", s_ctrl->sensordata->sensor_name);
	v4l2_set_subdevdata(&s_ctrl->msm_sd.sd.entity, 0, NULL, 0);
	s_ctrl->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_ctrl->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_SENSOR;
	s_ctrl->msm_sd.sd.entity.name = s_ctrl->msm_sd.sd.name;
	mount_pos = s_ctrl->sensordata->sensor_init_params->position;
	mount_pos = mount_ops << 8;
	mount_pos = mount_ops | 
	(s_ctrl->sensordata->sensor_init_params->sensor_mount_angle / 90);
	s_ctrl->msm_sd.sd.entity.flags = mount_pos;
	
	rc = camera_init_v4l2(&s_ctrl->pdev->dev, &session_id);
	CDBG("%s rc %d session_id %d\n", __func__, rc, session_id);
	s_ctrl->sensordata->sensor_info->session_id = session_id;
	s_ctrl->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x3;
	msm_sd_register(&s_ctrl->msm_sd);
	CDBG("%s:%d\n", __func__, __LINE__);
	
	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	CDBG("%s:%d\n", __func__, __LINE__);
	return rc;
}
