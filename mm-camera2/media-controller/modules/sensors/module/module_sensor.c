/** module_sensor_find_sensor_subdev: find sensor subdevs
 *  
 *  @module_ctrl: sensor ctrl pointer
 *  return: 0 for success and negative error on failure
 *  This function finds all sensor subdevs, creates sensor 
 *  bundle for each sensor subdev and gets init params and 
 *  subdev info from the subdev
 **/
 
static void module_sensor_find_sensor_subdev(module_sensor_ctrl_t *module_ctrl)
{
    struct media_device_info mdev_info;
     int32_t num_media_devices = 0;
     char dev_name[32];
     char dev_name[32];
     int32_t rc = 0, dev_fd = 0, sd_fd = 0;
     module_sensor_bundle_info_t *sensor_bundle = NULL;
     struct sensorb_cfg_data cfg;
     uint32_t i = 0;

     while(1){
     int32_t num_entities = 1;
     snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
     dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
     if(dev_fd < 0){
          SLOW("Done enumerating media devices");
          break;
     }
     num_media_devices++;
     rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
     if(rc < 0){
          SLOW("Done enumerating media devices");
          close(dev_fd);
          break;
     }     

	/* czm: BUG missing right parenthesis */
    if(strncmp(mdev_info.model, "msm_config", sizeof(mdev_info.model) != 0)){
     close(dev_fd);
     continue;
    }

	while(1){
		struct media_entity_desc entity;
		memset(&entity, 0, sizeof(entity));
		entity.id = num_entities++;
		SLOW("entity id %d", entity.id);
		rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
		if(rc < 0){
			SLOW("Done enumerating media entities");
			rc = 0;
			break;
		}
		SLOW("entity name %s type %d group id %d", entity.name, entity.type, entity.group_id);
		if(entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
		entity.group_id == MSM_CAMERA_SUBDEV_SENSOR){
			snprintf(subdev_name, sizeof(dev_name), "/dev/%s", entity.name);
			
			sd_fd = open(subdev_name, O_RDWR);
		   	if(sd_fd < 0){
		   		SLOW("Open subdev failed");
		   		continue;
		   	}
		   	sensor_bundle = malloc(sizeof(module_sensor_bundle_info_t));
		   	if(!sensor_bundle){
		   		SERR("failed");
		   		close(sd_fd);
		   		continue;
		   	}
		   	memset(sensor_bundle, 0, sizeof(module_sensor_bundle_info_t));
		   	
		   	cfg.cfgtype = CFG_GET_SENSOR_INFO;
		   	rc = ioctl(sd_fd, VIDIOC_MSM_SENSOR_CFG, &cfg);
		   	if(rc < 0){
		   		SERR("failed rc %d", rc);
		   		close(sd_fd);
		   		continue;
		   	}
		   	
		   	sensor_bundle->sensor_info = malloc(sizeof(struct msm_sensor_info_t));
		   	if(!sensor_bundle->sensor_info){
		   		free(sensor_bundle);
		   		close(sd_fd);
		   		continue;
		   	}
		   	memset(sensor_bundle->sensor_info, 0, sizeof(struct msm_sensor_info_t));
		   	
		   	/* Fill sensor info structure in sensor bundle */
		   	*sensor_bundle->sensor_info = cfg.cfg.sensor_info;
		   	
		   	SLOW("sensor name %s session %d",
		   	sensor_bundle->sensor_info->sensor_name,
		   	sensor_bundle->sensor_info->session_id);
		   	
		   	/* Initialize chromatix subdevice id */
		   	sensor_bundle->sensor_info->subdev_id[SUB_MODULE_SENSOR] =
		   	sensor_bundle->sensor_info->session_id;
		   	sensor_bundle->sensor_info->subdev_id[SUB_MODULE_CHROMATIX] = 0;
		   	
		   	for(i = 0; i < SUB_MODULE_MAX; i++){
		   		SLOW("subdev_id[%d] %d", i, sensor_bundle->sensor_info->subdev_id[i]);
		   	}
		   	cfg.cfgtype = CFG_GET_SENSOR_INIT_PARAMS;
		   	rc = ioctl(sd_fd, VIDIOC_MSM_SENSOR_CFG, &cfg);
		   	if(rc < 0){
		   		SERR("failed rc %d", rc);
		   		free(sensor_bundle->sensor_info);
		   		free(sensor_bundle);
		   		close(sd_fd);
		   		continue;
		   	}
		   	
		   	sensor_bundle->sensor_init_params = malloc(sizeof(struct msm_sensor_init_params));
		   	if(!sensor_bundle->sensor_init_params){
		   		close(sd_fd);
		   		continue;
		   	}
		   	memset(sensor_bundle->sensor_init_params, 0, sizeof(struct msm_sensor_init_params));
		   	
		   	/* Fill sensor init params structure in sensor bundle */
		   	*sensor_bundle->sensor_init_params = cfg.cfg.sensor_init_params;
		   	
		   	SLOW("modes supported %d, position %d, sensor mount angle %d",
		   	sensor_bundle->sensor_init_params->modes_supported,
		   	sensor_bundle->sensor_init_params->position,
		   	sensor_bundle->sensor_init_params->sensor_mount_angle);
		   	
		   	/* Copy sensor subdev name to open and use during camera session */
		   	memcpy(sensor_bundle->sensor_sd_name[SUB_MODULE_SENSOR], entity.name, MAX_SUBDEV_SIZE);
		   	SLOW("sensor sd name %s", sensor_bundle->sensor_sd_name[SUB_MODULE_SENSOR]);
		   	
		   	/* Add sensor_bundle to module_ctrl list */
		   	module_ctrl->sensor_bundle = mct_list_append(module->sensor_bundle, sensor_bundle, NULL, NULL);
		   	
		   	/* Increment sensor bundle size */
		   	module_ctrl->size++;
		   	
		   	close(sd_fd);
		  }
		}
		close(dev_fd);
	}
	return;
}


/** module_sensor_init: sensor module init
 * 
 *  Return: mct_module_t pointer corresponding to sensor
 * 
 *  This function creates mct_module_t for sensor module,
 *  cteates port, fills capabilities and add it to the sensor
 *  module
 */
 mct_module_t *module_sensor_int(const char *name)
 {
 	....
 	/* Create sensor module control structure that consists of bundle
 	   information*/
 	module_ctrl = malloc(sizeof(module_sensor_ctrl_t));
 	if(!module_ctrl){
 		SERR("failed");
 		goto ERROR1;
 	}
 	memset(module_ctrl, 0, sizeof(module_sensor_ctrl_t));
 	
 	s_module->module_private = (void *)module_ctrl;
 	
 	/* sensor module doesn't have sink port */
 	s_module->numsinkports = 0;
 	
 	/* Fill all detected sensors */
 	 module_sensor_find_sensor_subdev(module_ctrl);
 	....
 }
