void msm_isp_sof_notify(struct vfe_device *vfe_dev,
    enum msm_vfe_input_src frame_src, struct msm_isp_timestamp *ts) {
  struct msm_isp_event_data sof_event;
  uint32_t session_id;

  ...
  if((vfe_dev->axi_data.current_frame_src_mask[session_id] ==
      vfe_dev->axi_data.session_frame_src_mask[session_id])){
    vfe_dev->axi_data.current_frame_src_mask[session_id] = 0;

    ...
    sof_event.input_intf = vfe_dev->axi_data.session_frame_src_mask[session_id];
    sof_event.frame_id   = vfe_dev->axi_data.frame_id[session_id];
    sof_event.timestamp  = ts->event_time;
    sof_event.mono_timestamp = ts->buf_time;

    //czm send event to mm-camera isp module
    msm_isp_send_event(vfe_dev, ISP_EVENT_SOF + frame_scr, &sof_event);
    pr_debug("%s: frame id %d\n", __func__,
        vfe_dev->axi_data.frame_id[session_id]);
  }
}
