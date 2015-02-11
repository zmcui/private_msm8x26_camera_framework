int msm_isp_send_event(struct vfe_device *vfe_dev,
    uint32_t event_type,
    struct  msm_isp_event_data *event_data)
{
  struct v4l2_event isp_event;
  msmset(&isp_event, 0, sizeof(struct v4l2_event));
  isp_event.id = 0;
  isp_event.type = event_type;
  memcpy(&isp_event.u.data[0], event_data,
      sizeof(struct msm_isp_event_data));
  v4l2_event_queue(vfe_dev->subdev.sd.devnode, &isp_event);
  return 0;
}
