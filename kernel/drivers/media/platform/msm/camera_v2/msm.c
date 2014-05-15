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
#include <linux/of.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/videodev2.h>
#include <linux/msm_ion.h>
#include <linux/iommu.h>
#include <linux/platform_device.h>
#include <media/v4l2-fh.h>
#include "msm.h"
#include "msm_vb2.h"
#include "msm_sd.h"

/*czm: corespond to msm_config*/
static struct v4l2_device *msm_v4l2_dev;
static struct list_head   ordered_sd_list;

....
static inline int __msm_sd_register_subdev(struct v4l2_subdev *sd)
{
  int rc = 0;
  struct video_device *vdev;
  
  if(!msm_v4l2_dev || !sd || !sd->name[0])
    return -EINVAL;
    
  rc = v4l2_device_register_subdev(msm_v4l2_dev, sd);
  if(rc < 0)
    return rc;
    
  /* Register a device node for every subdev marked with the 
   * V4L2_SUBDEV_FL_HAS_DEVNODE flag
   */
  if(!(sd->flags & V4L2_SUBDEV_FL_HAS_DEVNODE))
    return rc;
    
  vdev = kzalloc(sizeof(*vdev), GFP_KERNEL);
  if(!vdev){
    rc = -ENOMEM;
    goto clean_up;
  }
  
  video_set_drvdata(vdev, sd);
  strlcpy(vdev->name, sd->name, sizeof(vdev->name));
  vdev->v4l2_dev = msm_v4l2_dev;
  vdev->fops = &v4l2_subdev_fops;
  vdev->release = msm_sd_unregister_subdev;
  rc = __video_register_device(vdev, VFL_TYPE_SUBDEV, -1, 1, sd->owner); 
  if(rc < 0){
    kzfree(vdev);
    goto clean_up;
  }
  
#if defined(CONFIG_MEDIA_CONTROLLER)
  sd->entity.info.v4l.major = VIDEO_MAJOR;
  sd->entity.info.v4l.minor = vdev->minor;
  sd->entity.name = video_device_node_name(vdev);
#endif
  sd->devnode = vdev;
  return 0;
  
clean up:
  if(sd->devnode)
    video_unregister_device(sd->devnode);
  return rc;
}

/* czm: this function is called in msm_vpe.c msm_cpp.c msm_generic_buf_mgr.c msm_ispif.c msm_eeprom.c 
 * msm_csiphy.c msm_csid.c msm_sensor.c msm_led_flash.c msm_cci.c msm_actuator.c msm_isp.c 
 */
int msm_sd_register(struct msm_sd_subdev *msm_subdev)
{
  if(WARN_ON(!msm_subdev))
    return -EINVAL;
    
  if(WARN_ON(!msm_v4l2_dev) || WARN_ON(!msm_v4l2_dev->dev))
    return -EIO;
    
  msm_add_sd_in_position(msm_subdev, &ordered_sd_list);
  return __msm_sd_register_subdev(&msm_subdev->sd);
}

static int __devinit msm_probe(struct platform_device *pdev)
{
  struct msm_video_device *pdev;
  int rc = 0;
  
  msm_v4l2_dev = kzalloc(sizeof(*msm_v4l2_dev), GFP_KERNEL);
  if(WARN_ON(!msm_v4l2_dev)){
    rc = -ENOMEM;
    goto probe_end;
  }
  
  pvdev = kzalloc(sizeof(struct msm_video_device), GFP_KERNEL);
  if(WARN_ON(!pvdev)){
    rc = -ENOMEM;
    goto pvdev_fail;
  }
  
  pvdev->vdev = video_device_alloc();
  if(WARN_ON(!pvdev->vdev)){
    rc = -ENOMEM;
    goto video_fail;
  }
    
#if defined(CONFIG_MEDIA_CONTROLLER)
  msm_v4l2_dev->mdev = kzalloc(sizeof(struct media_device), GFP_KERNEL);
  if(!msm_v4l2_dev->mdev){
    rc = -ENOMEM;
    goto = mdev_fail;
  }
    
  strlcpy(msm_v4l2_dev->mdev->model, MSM_CONFIGURATION_NAME,
          sizeof(msm_v4l2_dev->mdev->model));
  msm_v4l2_dev->mdev->dev = &(pdev->dev);
  
  rc = media_device_register(msm_v4l2_dev->mdev);
  if(WARN_ON(rc < 0))
    goto media_fail
  
  if (WARN_ON((rc == media_entity_init(&pvdev->vdev->entity,
          0, NULL, 0)) < 0))
      goto entity_fail;
      
  pvdev->vdev->entity.type = MEDIA_ENT_T_DEVNODE_V4L;
  pvdev->vdev->entity.group_id = QCAMERA_VNODE_GROUP_ID;
#endif

  msm_v4l2_dev->notify = msm_sd_notify;
  
  pvdev->vdev->v4l2_dev = msm_v4l2_dev;
  
  rc = v4l2_device_register(&(pdev->dev), pvdev->vdev->v4l2_dev);
  if (WARN_ON(rc < 0))
    goto register_fail;
    
  strlcpy(pvdev->vdev->name, "msm-config", sizeof(pvdev->vdev->name));
  pvdev->vdev->release  = video_device_release;
  pvdev->vdev->fops     = &msm_fops;
  pvdev->vdev->ioctl_ops = &g_msm_ioctl_ops;
  pvdev->vdev->minor     = -1;
  pvdev->vdev->vfl_type  = VFL_TYPE_GRABBER;
  rc = video_register_device(pvdev->vdev,
      VFL_TYPE_GRABBER, -1);
  if (WARN_ON(rc < 0))
    goto v4l2_fail;
    
#if defined(CONFIG_MEDIA_CONTROLLER)
  /* FIXME: How to get rid of this messy? */
  pvdev->vdev->entity.name = video_device_node_name(pvdev->vdev);
#endif

  atomic_set(&pvdev->opened, 0);
  video_set_drvdata(pvdev->vdev, pvdev);
  
  msm_session_q = kzalloc(sizeof(*msm_session_q), GFP_KERNEL);
  if (WARN_ON(!msm_session_q))
    goto v4l2_fail;
    
  msm_init_queue(msm_session_q);
  spin_lock_init(&msm_eventq_lock);
  spin_lock_init(&msm_pid_lock);
  INIT_LIST_HEAD(&ordered_sd_list);
  goto probe_end;
  
v4l2_fail:
  v4l2_device_unregister(pvdev->vdev->v4l2_dev);
register_fail:
#if defined(CONFIG_MEDIA_CONTROLLER)
  media_entity_cleanup(&pvdev->vdev->entity);
entity_fail:
  media_device_unregister(msm_v4l2_dev->mdev);
media_fail:
  kzfree(msm_v4l2_dev->mdev);
mdev_fail:
#endif
  video_device_release(pvdev->vdev);
video_fail:
  kzfree(pvdev);
pvdev_fail:
  kzfree(msm_v4l2_dev);
probe_end:
  return rc;
}

static const struct of_device_id msm_dt_match[] = {
  {.compatible = "qcom,msm-cam"},
}

MODULE_DEVICE_TABLE(of, msm_dt_match);

static struct platform_driver msm_driver = {
  .probe = msm_probe,
  .driver = {
    .name = "msm",
    .owner = THIS_MODULE,
    .of_match_table = msm_dt_match,
  },
};

static int __init msm_init(void)
{
  return platform_driver_register(&msm_driver);
}

static void __exit msm_exit(void)
{
  platform_driver_unregister(&msm_driver);
}

module_init(msm_init);
module_exit(msm_exit);
MODULE_DESCRIPTION("MSM V4L2 Camera");
MODULE_LICENSE("GPL v2");
