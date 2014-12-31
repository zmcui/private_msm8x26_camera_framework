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

/*==========================================================
 * FUNCTION   : get_sensor_info
 *
 * DESCRIPTION: get sensor info like facing(back/front) and mount angle
 *
 * PARAMETERS :
 *
 * RETURN     :
 *=========================================================*/
void get_sensor_info()
{
    int rc = 0;
    int dev_fd = 0;
    struct media_device_info mdev_info;
    int num_media_devices = 0;
    uint8_t num_cameras = 0;
    
    CDBG("%s : E", __func__);
    /*lock the mutex*/
    while(1){
        char dev_name[32];
        int num_entities;
        snprintf(dev_name, sizeof(dev_name), "/dev/media%d", num_media_devices);
        dev_fd = open(dev_name, O_RDWR | O_NONBLOCK);
        if(dev_id <= 0) {
            CDBG("Done discovering media devices");
            break;
        }
        num_media_devices++;
        memset(&mdev_info, 0, sizeof(mdev_info));
        rc = ioctl(dev_fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
        if(rc < 0){
            CDBG_ERROR("Error: ioctl media_dev failed: %s \n", strerror(errno));
            close(dev_fd);
            dev_fd = 0;
            num_cameras = 0;
            break;
        }
        
        /*MSM_CONFIGURATION_NAME defined in kernel/include/media/msmb_camera.h */
        if(strncmp(mdev_info.model, MSM_CONFIGURATION_NAME, sizeof(mdev_info.model)) != 0){
            close(dev_fd);
            dev_fd = 0;
            continue;
        }
        
        num_entities = 1;
        while(1){
            struct media_entity_desc entity;
            unsigned long temp;
            unsigned int mount_angle;
            unsigned int facing;
            
            memset(&entity, 0, sizeof(entity));
            entity.id = num_entities++;
            rc = ioctl(dev_fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
            if(rc < 0){
                CDBG("Done enumerating media entities\n");
                rc = 0;
                break;
            }
            if(entity.type == MEDIA_ENT_T_V4L2_SUBDEV &&
                entity.group_id == MSM_CAMERA_SUBDEV_SENSOR){
                temp = entity.flags >> 8;                 /*got from kernel entity*/
                mount_angle = (temp & 0xFF) * 90;
                facing = (temp >> 8);
                ALOGD("index = %d flag = %x mount_angle = %d facing = %d\n"
                    , num_cameras, (unsigned int)temp, (unsigned int)mount_angle, (unsigned int)facing);
                g_cam_ctrl.info[num_cameras].facing = facing;
                g_cam_ctrl.info[num_cameras].orientation = mount_angle;
                num_cameras++;
                continue;
            }
        }
        
        CDBG("%s: dev_info[id=%d, name='%s']\n",
            __func__, num_cameras, g_cam_ctrl.video_dev_name[num_cameras]);
            
        close(dev_fd);
        dev_fd = 0;
    }
    
    /*unlock the mutex*/
    CDBG("%s: num_cameras = %d\n", __func__, g_cam_ctrl.num_cam);
    return;
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



