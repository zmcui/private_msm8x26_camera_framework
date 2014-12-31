/*static function defination*/
/*used by RAW sensor*/
int32_t msm_sensor_driver_probe(void *setting)
{
...
    struct msm_camera_sensor_slave_info *slave_info = NULL;
...
    /* Validate input parameters */
    if(!setting){
        pr_err("failed: slave_info %p", setting);
        return -EINVAL;
    }

    /* Allocate memory for slave info */
    slave_info = kzalloc(sizeof(*slave_info), GFP_KERNEL);
    if(!slave_info){
        pr_err("failed: no memory slave_info %p", slave_info);
        return -ENOMEM;
    }
    
    if(copy_from_user(slave_info, (void *)setting, sizeof(*slave_info))){
        pr_err("failed: copy_from_user");
        rc = -EFAULT;
        goto FREE_SLAVE_INFO;
    }
    
    /* Print slave info */
    CDBG("camera id %d", slave_info->camera_id);
    CDBG("slave_addr 0x%x", slave_info->slave_addr);
    CDBG("addr_type %d", slave_info->addr_type);
...

    /* Update sensor mount angle and position in media entity flag */
    mount_pos = s_ctrl->sensordata->sensor_info->position << 16;
    mount_pos = mount_pos | ((s_ctrl->sensordata->sensor_info->
        sensor_mount_angle / 90) << 8);
    s_ctrl->msm_sd.sd.entity.flags = mount_pos | MEDIA_ENT_FL_DEFAULT;
    
    /* Save sensor info */
    s_ctrl->sensordata->cam_slave_info = slave_info;
    
    return rc;
...
}
