/** module_sensor_start_session:
 * 
 *  @module: sensor module
 *  @session_id: session id
 *
 *  Return: TRUE / FALSE
 */
static boolean module_sensor_start_session(
    mct_module_t *module, unsigned int sessionid)
{
  module_sensor_ctrl_t          *module_ctrl = NULL;
  mct_list_t                    *s_list = NULL;
  module_sensor_bundle_info_t   *s_bundle = NULL;
  boolean                       ret = TRUE;

  SHIGH("session %d", sessionid);
  if(!module) {
    SERR("failed");
    return FALSE;
  }

  module_ctrl = (module_sensor_ctrl_t *)module->module->private;
  if(!module_ctrl) {
    SERR("failed");
    return FALSE;
  }

  /* get the s_bundle from session id */
  s_list = mct_list_find_custom(module_ctrl->sensor_bundle, &sessionid,
      sensor_util_find_bundle);
  if(!s_list) {
    SERR("failed");
    return FALSE;
  }
  s_bundle = (module_sensor_bundle_info_t *)s_list->data;
  if(!s_bundle) {
    SERR("failed");
    return FALSE;
  }


  /* initialize the "torch on" flag to 0 */
  s_bundle->torch_on = 0;
  s_bundle->longshot = 0;

  /*
   * this init session includes
   * power up sensor, config init setting
   * */
  ret = module_sensor_init_session(s_bundle);
  if(ret == FALSE) {
    SERR("failed");
    return ERROR;
  }

  /*
   * create a sensor thread
   * */
  ret = sensor_thread_create(module);

  if(ret == FALSE) {
    SERR("failed");
    return ERROR;
  }

  return TRUE;

ERROR:
  SERR("failed");
  return FALSE;
}

/** module_sensor_hal_set_parm: process event for
 *  sensor_module
 * 
 *  @module_sensor_parms: pointer to sensor module params
 *  @event_control: pointer to control data that is sent with
 * 					S_PARM
 *  Return: TRUE / FALSE
 *  This function handles events associated with S_PARM 
 */
static boolean module_sensor_hal_set_parm(
	module_sensor_parms_t *module_sensor_params,
	mct_event_control_parm_t *event_control)
{
	boolean ret = TRUE;
	int32_t rc = SENSOR_SUCCESS;
	
	switch(event_control->type){
		case CAM_INTF_PARM_SATURATION:
		....
		case CAM_INTF_PARM_SHARPNESS:{
		  if(!event_control->parm_data){
		  	SERR("failed parm_data NULL");
		  	ret = FALSE;
		  	break;
		  }
		  /*czm this will call sensor_process() in sensor.c*/
		  rc = module_sensor_params->func_tbl.process(
		  	module_sensor_params->sub_module_private,
		  	SENSOR_SET_SHARPNESS, event_control->parm_data);
		  if(rc < 0){
		  	SERR("failed");
		  	ret = FALSE;
		  	break;
		  }
		}
		  break;
		....
	}
	....
}

/**  module_sensor_event_control_set_parm: process event for
 * 	 sensor module
 *
 * 	 @s_bundle: pointer to sensor bundle
 *   @control_data: pointer to control data that is sent with
 * 					S_PARM
 * 
 *   Return: TRUE / FALSE
 * 
 *   This function handles all events associated with S_PARM
 */
static boolean module_sensor_event_control_set_parm(
	mct_module_t *module, mct_event_t* event,
	sensor_bundle_info_t *bundle)
{
	....
	default:{
		sensor_output_format_t output_format;
		rc = module_sensor_params->func_tbl.process(
			module_sensor_params->sub_module_private,
			SENSOR_GET_SENSOR_FORMAT, &output_format);
		if(output_format == SENSOR_YCBCR){
			ret = module_sensor_hal_set_parm(module_sensor_params, event_control);
		}
	}
	break;
	....
}

/** module_sensor_process_event: process event for sensor
 *  module
 * 
 *  @streamid: streamid associated with event
 *  @module: mct module handle
 *  @event: event to be processed
 * 
 *  Return: 0 for success and negative error on failure
 * 
 *  This function handles all events and sends those events
 *  downstream / upstream **/
static boolean module_sensor_module_process_event(mct_module_t *module,
mct_event_t *event)
{
	boolean		ret = TRUE;
	int32_t		rc = SENSOR_SUCCESS;
	module_sensor_ctrl_t *module_ctrl = NULL;
	mct_event_control_t *event_ctrl = NULL;
	sensor_bundle_infor_t bundle_info;
	
	if(!module || !event){
		SERR("failed port %p event %p", module, event);
		return FALSE;
	}
	if(event->type != MCT_EVENT_CONTROL_CMD){
		SERR("failed invalid event type %d", event->type);
		return FALSE;
	}
	
	module_ctrl = (module_sensor_ctrl_t *)module->module_private;
	if(!module_ctrl){
		SERR("failed");
		return FALSE;
	}
	
	event_ctrl = &event->u.ctrl_event;
	
	memset(&bundle_info, 0, sizeof(sensor_bundle_info_t));
	ret = sensor_util_get_bundle(module, event->identity, &bundle_info);
	if(ret == FALSE){
		SERR("failed");
		return FALSE;
	}
	SLOW("event id %d", event_ctrl->type);
	
	if(event_ctrl->type == MCT_EVENT_CONTROL_PREPARE_SNAPSHOT){
		sensor_output_format_t output_format;
		mct_bus_msg_t bus_msg;
		module_sensor_params_t *module_sensor_params = NULL;
		
		bundle_info.s_bundle->state = 0;
		bundle_info.s_bundle->regular_led_triger = 0;
		module_sensor_params = bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
		rc = module_sensor_params->func_tbl.process(
			module_sensor_params->sub_module_private,
			SENSOR_GET_SENSOR_FROMAT, &output_format);
		SLOW("in Prepare snapshot, sensor type is %d \n", output_format);
		if(output_format == SENSOR_YCBCR){
			bus_msg.sessionid = bundle_info.s_bundle->sensor_info->session_id;
			bus_msg.type = MCT_BUS_MSG_PREPARE_HW_DONE;
			cam_prep_snapshot_state_t state;
			state = DO_NOT_NEED_FUTURE_FRAME;
			bus_msg.msg = &state;
			if(mct_module_post_bus_msg(module, &bus_msg) != TRUE)
			  SERR("Failure posting to the bus!");
			return TRUE;
		}
	}
	switch(event_ctrl->type){
		case MCT_EVENT_CONTROL_STREAMON;
		  SLOW("CT_EVENT_CONTROL_STREAMON");
		  memcpy(&module_ctrl->streaminfo, event->u.ctrl_event.control_event_data, sizeof(mct_stream_info_t));
		  ret = module_sensor_stream_on(module, event, bundle_info.s_bundle);
		  if(ret == FALSE){
		  	SERR("failed");
		  	break;
		  }
		  break;
		case ....
		case MCT_EVENT_CONTROL_SET_PARM:{
			ret = module_sensor_event_control_set_parm(
				module, event, &bundle_info);
			
			if(ret == FALSE){
				SERR("failed");
			}
			mct_event_control_parm_t *event_control = 
			  (mct_event_control_parm_t *)(event->u.ctrl_event.control_event_data);
			
			module_sensor_parms_t *module_sensor_parms = 
			  bundle_info.s_bundle->module_sensor_params[SUB_MODULE_SENSOR];
			sensor_bracket_params_t *af_bracket_params = 
			  &(bundle_info.s_bundle->af_bracket_params);
			sensor_bracket_params_t *flash_bracket_params =
			  &(bundle_info.s_bundle->flash_bracket_params);
			  
			sensor_output_format_t output_format;
			rc = module_sensor_params->func_tbl.process(
				module_sensor_params->sub_module_private,
				SENSOR_GET_SENSOR_FORMAT, &output_format);
			if(output_format == SENSOR_BAYER ||
				(event_control->type == CAM_INTF_PARM_ZOOM) ||
				(event_control->type == CAM_INTF_PARM_FD)){
					
				// Frame skip during bracketing must not be forwarded
				if((CAM_INTF_PARM_FRAMESKIP != event_control->type) &&
				   (!af_bracket_params->ctrl.enable)&&
				   (!flash_bracket_params->ctrl.enable)){
				   	
				   	/* Call send_event to propogate event to next module*/
				   	ret = sensor_util_post_event_in_src_port(module, event);
				   	if(ret == FALSE){
				   		SERR("failed");
				   		return FALSE;
				   	}
				   }
				}
				break;
			}
			....
			default:
			/* Call send_event to propogate to next module */
			ret = sensor_util_post_event_on_src_port(module, event);
			if(ret == FALSE){
				SERR("failed");
				return FALSE;
			}
		break;	
	}// switch end
	return ret;
}

/** module_sensor_set_mod: set mod function for sensor module
 *
 *	This function handles set mod events sent by mct **/
static void module_sensor_set_mod(mct_module_t *module,
	unsigned int module_type, unsigned int identity)
{
	SLOW("Enter, module_type=%d", module_type);
	mct_module_add_type(module, module_type, identity);
	if(module_type == MCT_MODULE_FLAG_SOURCE){
		mct_module_set_process_event_func(module,
			module_sensor_module_process_event);
	}
	return;
}


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
 	boolean					ret = TRUE;
 	mct_module_t			*s_module = NULL;
 	module_sensor_ctrl_t 	*module_ctrl = NULL;
 	
 	SHIGH("Enter");
 	
 	/* Create MCT module for sensor */
 	s_module = mct_module_create(name);
 	if(!s_module){
 		SERR("failed");	
 		return NULL;
 	}
 	
 	/* Fill function table in MCT module */
 	s_module->set_mod = module_sensor_set_mod;
 	s_module->query_mod = module_sensor_query_mod;
 	s_module->start_session = module_sensor_start_session;
 	s_module->stop_session = module_sensor_stop_session;
 	
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
