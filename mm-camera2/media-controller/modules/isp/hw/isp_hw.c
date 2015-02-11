/*
 * isp_hw_proc_subdev_event
 *
 *  @isp_hw
 *  @thread_data:
 *
 * */
void isp_hw_proc_subdev_event(isp_hw_t *isp_hw, isp_thread_t *thread_data)
{
  ...
  switch(v4l2_event.type) {
  case ISP_EVENT_STATS_NOTIFY:{
  ...
  }
  //from kernel/.../msm_isp_axi_util.c msm_isp_sof_notify()
  case ISP_EVENT_SOF:{
  ...
  bus_msg.type = MCT_BUS_MSG_ISP_SOF;
  bus_msg.msg = (void *)&sof_event;
  sof_event.frame_id = sof->frame_id;
  sof_event.timestamp = sof->timestamp;
  sof_event.mono_timestamp = sof->mono_timestamp;
  bus_msg.sessionid = isp_hw->pipeline.session_id[input_src];
  ...
  }
  }

}
