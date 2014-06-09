/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
static int camera_v4l2_s_ctrl(struct file *filep, void *fh,
	struct v4l2_control *ctrl)
{
	int rc = 0;
	struct v4l2_event event;
	struct msm_v4l2_event_data *event_data;
	if (ctrl->id >= V4L2_CID_PRIVATE_BASE) {
		camera_pack_event(filep, MSM_CAMERA_SET_PARM, ctrl->id,
		ctrl->value, &event);
		
		/*czm MSM_CAMERA_SET_PARM will be processed by mct_pipeline_process_serv_msg() in mct_pipeline of mm-camera2*/
		rc = msm_post_event(&event, MSM_POST_EVT_TIMEOUT);
		if (rc < 0)
			return rc;
		event_data = (struct msm_v4l2_event_data *)event.u.data;
		ctrl->value = event_data->ret_value;
		rc = camera_check_event_status(&event);
	}
	
	return rc;
}



static const struct v4l2_ioctl_ops camera_v4l2_ioctl_ops = {
	.vidioc_querycap = camera_v4l2_querycap,
	.vidioc_s_crop = camera_v4l2_s_crop,
	.vidioc_g_crop = camera_v4l2_g_crop,
	.vidioc_queryctrl = camera_v4l2_queryctrl,
	.vidioc_g_ctrl = camera_v4l2_g_ctrl,
	.vidioc_s_ctrl = camera_v4l2_s_ctrl,
	....
}

int camera_init_v4l2(struct device *dev, unsigned int *session)
{
	struct msm_video_device *pvdev;
	struct v4l2_device *v4l2_dev;
	int rc = 0;
	
	pvdev = kzalloc(sizeof(struct msm_video_device), GFP_KERNEL);
	if(WARN_ON(!pvdev)){
		rc = -ENMEM;
		goto init_end;
	}
	
	pvdev->vdev = video_device_alloc();
	if(WARN_ON(!pvdev->vdev)){
		rc = -ENOMEM;
		goto v4l2_fail;
	}
	
#if defined(CONFIG_MEDIA_CONTROLLER)
	v4l2_dev->mdev = kzalloc(sizeof(struct media_device), GFP_KERNEL);
	if(!v4l2_dev->mdev){
		rc = -ENOMEM;
		goto mdev_fail;
	}
	strlcpy(v4l2_dev->mdev->model, MSM_CAMERA_NAME, sizeof(v4l2_dev->mdev->model));
	
	v4l2_dev->mdev->dev = dev;
	
	rc = media_device_register(v4l2_dev->mdev);
	if(WARN_ON(rc < 0))
		goto media_fail;
		
	rc = media_entity_init(&pvdev->vdev->entity, 0, NULL, 0);
	if(WARN_ON(rc < 0))
		goto entity_fail;
	pvdev->vdev->entity.type = MEDIA_ENT_T_DEVNODE_V4L;
	pvdev->vdev->entity.group_id = QCAMERA_VNODE_GROUP_ID;
#endif

	v4l2_dev->notify = NULL;
	pvdev->vdev->v4l2_dev = v4l2_dev;
	
	rc = v4l2_device_register(dev, pvdev->vdev->v4l2_dev);
	if(WARN_ON(rc < 0))
		goto register_fail;
		
	strlcpy(pvdev->vdev->name, "msm-sensor", sizeof(pvdev->vdev->name));
	pvdev->vdev->release = video_device_release;
	pvdev->vdev->fops = &camera_v4l2_fops;
	pvdev->vdev->ioctl_ops = &camera_v4l2_ioctl_ops;
	pvdev->vdev->minor = -1;
	pvdev->vdev->vfl_type = VFL_TYPE_GRABBER;
	rc = video_register_device(pvdev->vdev, VFL_TYPE_GRABBER, -1);
	if(WARN_ON(rc < 0))
		goto video_register_fail;
#if defined(CONFIG_MEDIA_CONTROLLER);
	/* fixme: how to get rid of this messy? */
	pvdev->vdev->entity.name = video_device_node_name(pvdev->vdev);
#endif

	*session = pvdev->vdev->num;
	atomic_set(&pvdev->opened, 0);
	atomic_set(&pvdev->stream_cnt, 0);
	video_set_drvdata(pvdev->vdev, pvdev);
	device_init_wakeup(&pvdev->vdev->dev, 1);
	goto init_end;
	
video_register_fail:
	v4l2_device_unregister(pvdev->vdev->v4l2_dev);
register_fail:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&pvdev->vdev->entity);
entity_fail:
	media_device_unregister(v4l2_dev->mdev);
media_fail:
	kzfree(v4l2_dev->mdev);
mdev_fail:
#endif
	kzfree(v4l2_dev);
v4l2_fail:
	video_device_release(pvdev->vdev);
video_fail:
	kzfree(pvdev);
inti_end:
	return rc;
}
